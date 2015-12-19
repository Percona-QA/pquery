/*
 =========================================================
 #       Created by Alexey Bychko, Percona LLC           #
 #     Expanded by Roel Van de Paar, Percona LLC         #
 =========================================================
*/

#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cerrno>
#include <thread>                                 /* c++11 or gnu++11 */
#include <fstream>
#include <sstream>

#include <sys/time.h>
#include <unistd.h>
#include <getopt.h>

#include <my_global.h>
#include <mysql.h>

#include "pquery.hpp"

using namespace std;

static int verbose;
static int log_all_queries;
static int log_failed_queries;
static int no_shuffle;
static int query_analysis;
static int log_query_duration;
static int test_connection;

char db[] = "test";
char sock[] = "/tmp/my.sock";
char sqlfile[] = "pquery.sql";
char outdir[] = "/tmp";

struct conndata
{
  char database[255];
  char addr[255];
  char socket[255];
  char username[255];
  char password[255];
  char infile[255];
  char logdir[255];
  int port;
  int threads;
  int queries_per_thread;
} m_conndata;

inline unsigned long long
get_affected_rows(MYSQL * connection){
  if (mysql_affected_rows(connection) == ~(ulonglong) 0){
    return 0LL;
  }
  return mysql_affected_rows(connection);
}


void try_connect() {
  MYSQL * conn;
  conn = mysql_init(NULL);
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    printf("* PQUERY: Unable to continue [1], exiting\n");
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  if (mysql_real_connect(conn, m_conndata.addr, m_conndata.username,
  m_conndata.password, m_conndata.database, m_conndata.port, m_conndata.socket, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    printf("* PQUERY: Unable to continue [2], exiting\n");
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  printf("- PQuery v%s compiled with %s-%s \n", PQVERSION, FORK, mysql_get_client_info());
  printf("- Connected to %s...\n", mysql_get_host_info(conn));
// getting the real server version
  MYSQL_RES *result = NULL;
  string server_version;

  if (!mysql_query(conn, "select @@version_comment limit 1") && (result = mysql_use_result(conn))) {
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]) {
      server_version = mysql_get_server_info(conn);
      server_version.append(" ");
      server_version.append(row[0]);
    }
  }
  else {
    server_version = mysql_get_server_info(conn);
  }
  printf("- Connected server version: %s \n", server_version.c_str());
  if (result != NULL) {
    mysql_free_result(result);
  }
  mysql_close(conn);
  if(test_connection){
    printf("- Ending test run\n");
    mysql_library_end();
    exit(0);
  }
}

void executor(int number, const vector<string>& qlist) {
  if(verbose) {
    printf("Thread %d started\n", number);
  }

  int failed_queries = 0;
  int total_queries = 0;
  int max_con_failures = 250;                     /* Maximum consecutive failures (likely indicating crash/assert, user priveleges drop etc.) */
  int max_con_fail_count = 0;
  int res;
  struct timeval begin, end;
  double elapsed;
  FILE * thread_log = NULL;

  if ((log_failed_queries) || (log_all_queries) || (query_analysis)) {
    ostringstream os;
    os << m_conndata.logdir << "/pquery_thread-" << number << ".log";
    thread_log = fopen(os.str().c_str(), "w+");
  }

  MYSQL * conn;

  conn = mysql_init(NULL);
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));

    if (thread_log != NULL) {
      fclose(thread_log);
    }
    printf("Thread #%d is exiting\n", number);
    return;
  }
  if (mysql_real_connect(conn, m_conndata.addr, m_conndata.username,
  m_conndata.password, m_conndata.database, m_conndata.port, m_conndata.socket, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    mysql_close(conn);
    if (thread_log != NULL) {
      fclose(thread_log);
    }
    mysql_thread_end();
    return;
  }

  for (int i=0; i<m_conndata.queries_per_thread; i++) {
    int query_number;
    if(no_shuffle) {
      query_number = i;
    }
    else {
      struct timeval t1;
      gettimeofday(&t1, NULL);
      unsigned int seed = t1.tv_usec * t1.tv_sec;
      srand(seed);
      query_number = rand() % qlist.size();
    }

// perform the query and getting the result

    if(log_query_duration) {
      gettimeofday(&begin, NULL);
    }

    res = mysql_real_query(conn, qlist[query_number].c_str(), (unsigned long)strlen(qlist[query_number].c_str()));

    if(log_query_duration) {
      gettimeofday(&end, NULL);
                                                  // time in milliseconds (1/1000 sec)
      elapsed = ((end.tv_sec - begin.tv_sec)*1000) + ((end.tv_usec - begin.tv_usec)/1000.0);
    }

    if (res == 0) {                               //success
      max_con_fail_count=0;
    }
    else {
      failed_queries++;
      max_con_fail_count++;
      if (max_con_fail_count >= max_con_failures) {
        printf("* Last %d consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.\n", max_con_fail_count);
        if (thread_log != NULL) {
          fprintf(thread_log,"# Last %d consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.\n", max_con_fail_count);
        }
        break;
      }
    }

    total_queries++;
    MYSQL_RES * result = mysql_store_result(conn);

// logging part, initial implementation, will be refactored / rewritten
    if((verbose) && (m_conndata.threads == 1)) { // print it only if 1 thread is active

      if(res == 0) {
        if( (log_all_queries) || (query_analysis) ){
          fprintf(stderr, "%s", qlist[query_number].c_str());
          fprintf(stderr, " # NOERROR");
          if(query_analysis){
            fprintf(stderr, " # WARNINGS: %u", mysql_warning_count(conn));
            fprintf(stderr, " # CHANGED: %llu", get_affected_rows(conn) );
          }
          if(log_query_duration) {
            fprintf(stderr, " # Duration: %f msec", elapsed);
          }
          fprintf(stderr, "\n");
        }
      } else {
        if((log_failed_queries) || (log_all_queries) || (query_analysis)) {
          fprintf(stderr, "%s", qlist[query_number].c_str());
          fprintf(stderr, " # ERROR:");
          fprintf(stderr, " %u - %s", mysql_errno(conn), mysql_error(conn));
          if(query_analysis){
            fprintf(stderr, " # WARNINGS: %u", mysql_warning_count(conn));
            fprintf(stderr, " # CHANGED: %llu", get_affected_rows(conn) );
          }
           if(log_query_duration) {
            fprintf(stderr, " # Duration: %f msec", elapsed);
          }
          fprintf(stderr, "\n");
        }
      } //if / else
    }

//
    if(thread_log != NULL) {
      if(res == 0) {
        if((log_all_queries) || (query_analysis)) {
          fprintf(thread_log, "%s", qlist[query_number].c_str());
          fprintf(thread_log, " # NOERROR");
          if(query_analysis){
            fprintf(thread_log, " # WARNINGS: %u", mysql_warning_count(conn));
            fprintf(thread_log, " # CHANGED: %llu", get_affected_rows(conn));
          }
          if(log_query_duration) {
            fprintf(thread_log, " # Duration: %f msec", elapsed);
          }
          fprintf(thread_log, "\n");
        }
      }
      else {
        if((log_failed_queries) || (log_all_queries) || (query_analysis)) {
          fprintf(thread_log, "%s", qlist[query_number].c_str());
          fprintf(thread_log, " # ERROR:");
          fprintf(thread_log, " %u - %s", mysql_errno(conn), mysql_error(conn));
          if(query_analysis){
            fprintf(thread_log, " # WARNINGS: %u", mysql_warning_count(conn));
            fprintf(thread_log, " # CHANGED: %llu", get_affected_rows(conn) );
          }
          if(log_query_duration) {
            fprintf(thread_log, " # Duration: %f msec", elapsed);
          }
          fprintf(thread_log, "\n");
        }
      }
    }
    if (result != NULL) {
      mysql_free_result(result);
    }
  }                                               //for loop

  printf("* SUMMARY: %d/%d queries failed (%.2f%% were successful)\n", failed_queries, total_queries, (total_queries-failed_queries)*100.0/total_queries);
  if (thread_log != NULL) {
    fprintf(thread_log,"# SUMMARY: %d/%d queries failed (%.2f%% were successful)\n", failed_queries, total_queries, (total_queries-failed_queries)*100.0/total_queries);
    fclose(thread_log);
  }
  mysql_close(conn);
  mysql_thread_end();
}

int main(int argc, char* argv[]) {

  m_conndata.threads = 10;
  m_conndata.port = 0;
  m_conndata.queries_per_thread = 10000;

  int c;

  strncpy(m_conndata.database, db, strlen(db) + 1);
  strncpy(m_conndata.socket, sock, strlen(sock) + 1);
  strncpy(m_conndata.infile, sqlfile, strlen(sqlfile) + 1);
  strncpy(m_conndata.logdir, outdir, strlen(outdir) + 1);

  while(true) {

    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"database", required_argument, 0, 'd'},
      {"address", required_argument, 0, 'a'},
      {"infile", required_argument, 0, 'i'},
      {"logdir", optional_argument, 0, 'l'},
      {"socket", required_argument, 0, 's'},
      {"port", required_argument, 0, 'p'},
      {"user", required_argument, 0, 'u'},
      {"password", required_argument, 0, 'P'},
      {"threads", required_argument, 0, 't'},
      {"queries-per-thread", required_argument, 0, 'q'},
      {"verbose", no_argument, &verbose, 1},
      {"log-all-queries", no_argument, &log_all_queries, 1},
      {"log-failed-queries", no_argument, &log_failed_queries, 1},
      {"no-shuffle", no_argument, &no_shuffle, 1},
      {"query-analysis", no_argument, &query_analysis, 1},
      {"log-query-duration", no_argument, &log_query_duration, 1},
      {"test-connection", no_argument, &test_connection, 1},
      {0, 0, 0, 0}
    };

    int option_index = 0;

    c = getopt_long_only(argc, argv, "d:a:i:l:s:p:u:P:t:q", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 'h':
        show_help();
        exit(EXIT_FAILURE);
      case 'd':
        printf("Database is %s\n", optarg);
        memcpy(m_conndata.database, optarg, strlen(optarg) + 1);
        break;
      case 'a':
        printf("Address is %s\n", optarg);
        memcpy(m_conndata.addr, optarg, strlen(optarg) + 1);
        break;
      case 'i':
        printf("Infile is %s\n", optarg);
        memcpy(m_conndata.infile, optarg, strlen(optarg) + 1);
        break;
      case 'l':
        printf("Logdir is %s\n", optarg);
        memcpy(m_conndata.logdir, optarg, strlen(optarg) + 1);
        break;
      case 's':
        printf("Socket is %s\n", optarg);
        memcpy(m_conndata.socket, optarg, strlen(optarg) + 1);
        break;
      case 'p':
        printf("Port is %s\n", optarg);
        m_conndata.port = atoi(optarg);
        break;
      case 'u':
        printf("User is %s\n", optarg);
        memcpy(m_conndata.username, optarg, strlen(optarg) + 1);
        break;
      case 'P':
        printf("Password is %s\n", optarg);
        memcpy(m_conndata.password, optarg, strlen(optarg) + 1);
        break;
      case 't':
        printf("Starting with %s thread(s)\n", optarg);
        m_conndata.threads = atoi(optarg);
        break;
      case 'q':
        printf("Query limit per thread is %s\n", optarg);
        m_conndata.queries_per_thread = atoi(optarg);
        break;
      default:
        break;
    }
  }                                               //while

  // try to connect and print server info
  try_connect();

  ifstream infile;
  infile.open(m_conndata.infile);

  if (!infile) {
    printf("Unable to open SQL file %s: %s\n", m_conndata.infile, strerror(errno));
    exit(EXIT_FAILURE);
  }

  shared_ptr<vector<string>> querylist(new vector<string>);
  string line;

  while (getline(infile, line)) {
    if(!line.empty()) {
      querylist->push_back(line);
    }
  }
  infile.close();

/* log replaying */
  if(no_shuffle) {
    m_conndata.threads = 1;
    m_conndata.queries_per_thread = querylist->size();
  }
/* END log replaying */
  vector<thread> threads;
  threads.clear();
  threads.resize(m_conndata.threads);

  for (int i=0; i<m_conndata.threads; i++) {
    threads[i] = thread(executor, i, *querylist);
  }

  for (int i=0; i<m_conndata.threads; i++) {
    threads[i].join();
  }
  
  mysql_library_end();

  return EXIT_SUCCESS;
}
