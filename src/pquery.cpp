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
#include <thread>                /* c++11 or gnu++11 */
#include <string>
#include <fstream>
#include <sstream>

#include <unistd.h>
#include <getopt.h>

#include <my_global.h>
#include <mysql.h>

#include "pquery.hpp"

using namespace std;

char db[] = "test";
char sock[] = "/var/run/mysqld/mysqld.sock";
char sqlfile[] = "pquery.sql";
char outdir[] = "/tmp";

static struct conndata
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
  int verbose;
  int log_all_queries;
  int log_failed_queries;
  int no_shuffle;
  int query_analysis;
} m_config;

void executor(int number, const vector<string>& qlist) {
  if(m_config.verbose) {
    printf("Thread %d started\n", number);
  }

  int failed_queries = 0;
  int total_queries = 0;
  int max_con_failures = 250;    /* Maximum consecutive failures (likely indicating crash/assert, user privileges drop etc.) */
  int max_con_fail_count = 0;

  FILE * thread_log = NULL;

  if ((m_config.log_failed_queries) || (m_config.log_all_queries)) {
    ostringstream os;
    os << m_config.logdir << "/pquery_thread-" << number << ".sql";
    thread_log = fopen(os.str().c_str(), "w+");
  }

  MYSQL * conn;

  conn = mysql_init(NULL);
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));

    if (thread_log != NULL) {
      fclose(thread_log);
    }
    if(m_config.verbose){
      printf("Thread #%d is exiting\n", number);
    }
    return;
  }
  if (mysql_real_connect(conn, m_config.addr, m_config.username,
  m_config.password, m_config.database, m_config.port, m_config.socket, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    mysql_close(conn);
    if (thread_log != NULL) {
      fclose(thread_log);
    }
    mysql_thread_end();
    return;
  }

  for (int i=0; i<m_config.queries_per_thread; i++) {
    int query_number;
    if(m_config.no_shuffle) {
      query_number = i;
    }
    else {
      struct timeval t1;
      gettimeofday(&t1, NULL);
      unsigned int seed = t1.tv_usec * t1.tv_sec;
      srand(seed);
      query_number = rand() % qlist.size();
    }
    if (m_config.log_all_queries) {
      fprintf(thread_log, "%s\n", qlist[query_number].c_str());
    }

    if (mysql_real_query(conn, qlist[query_number].c_str(), (unsigned long)strlen(qlist[query_number].c_str()))) {
      failed_queries++;

      if(m_config.verbose) {
        fprintf(stderr, "# Query: \"%s\" FAILED: %s\n", qlist[query_number].c_str(), mysql_error(conn));
      }
      if (m_config.log_failed_queries) {
        fprintf(thread_log, "# Query: \"%s\" FAILED: %s\n", qlist[query_number].c_str(), mysql_error(conn));
      }
      max_con_fail_count++;
      if (max_con_fail_count >= max_con_failures) {
        printf("* Last %d consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.\n", max_con_fail_count);
        if (thread_log != NULL) {
          fprintf(thread_log,"# Last %d consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.\n", max_con_fail_count);
        }
        break;
      }
    }
    else {
      if(m_config.verbose) {
        fprintf(stderr, "%s\n", qlist[query_number].c_str());
      }
      max_con_fail_count=0;
    }
    total_queries++;
    MYSQL_RES * result = mysql_store_result(conn);
    if (result != NULL) {
      const char* qres = mysql_info(conn);
      if (qres != NULL){
        fprintf(stderr, "%s\n", qres);
      }
      mysql_free_result(result);
    }
  }

  printf("* Thread #%d SUMMARY: %d/%d queries failed (%.2f%% were successful)\n", number, failed_queries, total_queries, (total_queries-failed_queries)*100.0/total_queries);
  if (thread_log != NULL) {
    fprintf(thread_log,"# SUMMARY: %d/%d queries failed (%.2f%% were successful)\n", failed_queries, total_queries, (total_queries-failed_queries)*100.0/total_queries);
    fclose(thread_log);
  }
  mysql_close(conn);
  mysql_thread_end();
}


int main(int argc, char* argv[]) {

  m_config.threads = 10;
  m_config.port = 0;
  m_config.queries_per_thread = 10000;

  int c, option_index = 0;

  strncpy(m_config.database, db, strlen(db) + 1);
  strncpy(m_config.socket, sock, strlen(sock) + 1);
  strncpy(m_config.infile, sqlfile, strlen(sqlfile) + 1);
  strncpy(m_config.logdir, outdir, strlen(outdir) + 1);

  static struct option long_options[] = {
    {"help", no_argument, 0, 'h'},
    {"database", required_argument, 0, 'd'},
    {"address", required_argument, 0, 'a'},
    {"infile", required_argument, 0, 'i'},
    {"logdir", required_argument, 0, 'l'},
    {"socket", required_argument, 0, 's'},
    {"port", required_argument, 0, 'p'},
    {"user", required_argument, 0, 'u'},
    {"password", required_argument, 0, 'P'},
    {"threads", required_argument, 0, 't'},
    {"queries-per-thread", required_argument, 0, 'q'},
    {"verbose", no_argument, &m_config.verbose, 1},
    {"log-all-queries", no_argument, &m_config.log_all_queries, 1},
    {"log-failed-queries", no_argument, &m_config.log_failed_queries, 1},
    {"query-analysis", no_argument, &m_config.query_analysis, 1},
    {"no-shuffle", no_argument, &m_config.no_shuffle, 1},
    {0, 0, 0, 0}
  };

  while( (c = getopt_long_only(argc, argv, "d:a:i:l:s:p:u:P:t:q", long_options, &option_index)) != -1){
    switch (c) {
      case 'h':
        show_help();
        exit(0);
        break; // yes, I know
      case 'd':
        printf("Database is %s\n", optarg);
        memcpy(m_config.database, optarg, strlen(optarg) + 1);
        break;
      case 'a':
        printf("Address is %s\n", optarg);
        memcpy(m_config.addr, optarg, strlen(optarg) + 1);
        break;
      case 'i':
        printf("Infile is %s\n", optarg);
        memcpy(m_config.infile, optarg, strlen(optarg) + 1);
        break;
      case 'l':
        printf("Logdir is %s\n", optarg);
        memcpy(m_config.logdir, optarg, strlen(optarg) + 1);
        break;
      case 's':
        printf("Socket is %s\n", optarg);
        memcpy(m_config.socket, optarg, strlen(optarg) + 1);
        break;
      case 'p':
        printf("Port is %s\n", optarg);
        m_config.port = atoi(optarg);
        break;
      case 'u':
        printf("User is %s\n", optarg);
        memcpy(m_config.username, optarg, strlen(optarg) + 1);
        break;
      case 'P':
        printf("Password is %s\n", optarg);
        memcpy(m_config.password, optarg, strlen(optarg) + 1);
        break;
      case 't':
        printf("Starting with %s threads\n", optarg);
        m_config.threads = atoi(optarg);
        break;
      case 'q':
        printf("Query limit per thread is %s\n", optarg);
        m_config.queries_per_thread = atoi(optarg);
        break;
      default:
        break;
    }
  }                              //while

  // turning all logging on if query analysis is set
  if (m_config.query_analysis){
    m_config.log_all_queries = 1;
    m_config.log_failed_queries = 1;
  }

  MYSQL * conn;
  conn = mysql_init(NULL);
  if (conn == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    printf("* PQUERY: Unable to continue [1], exiting\n");
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }

  if (mysql_real_connect(conn, m_config.addr, m_config.username,
  m_config.password, m_config.database, m_config.port, m_config.socket, 0) == NULL) {
    printf("Error %u: %s\n", mysql_errno(conn), mysql_error(conn));
    printf("* PQUERY: Unable to continue [2], exiting\n");
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }

  if (m_config.verbose) {
    printf("- Connected to server (%s)... \n", mysql_get_host_info(conn));
    printf("- PQuery v%s compiled with %s-%s \n", PQVERSION, FORK, mysql_get_client_info());
  }

  // getting the real server version
  MYSQL_RES *result = NULL;
  string server_version;

  if (!mysql_query(conn, "select @@version_comment limit 1") && (result = mysql_use_result(conn))) {
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row && row[0]){
      server_version = mysql_get_server_info(conn);
      server_version.append(" ");
      server_version.append(row[0]);
    }
  } else {
    server_version = mysql_get_server_info(conn);
  }
  if (m_config.verbose){
    printf("- Connected server version: %s \n", server_version.c_str());
  }
  if (result){
    mysql_free_result(result);
  }
  mysql_close(conn);

  ifstream infile;
  infile.open(m_config.infile);

  if (!infile) {
    printf("Unable to open SQL file %s: %s\n", m_config.infile, strerror(errno));
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
  if(m_config.no_shuffle) {
    m_config.threads = 1;
    m_config.queries_per_thread = querylist->size();
  }
  /* END log replaying */
  vector<thread> threads;
  threads.clear();
  threads.resize(m_config.threads);

  for (int i=0; i<m_config.threads; i++) {
    threads[i] = thread(executor, i, *querylist);
  }

  for (int i=0; i<m_config.threads; i++) {
    threads[i].join();
  }

  mysql_library_end();

  return EXIT_SUCCESS;
}
