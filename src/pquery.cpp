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
#include <iostream>

#include <chrono>
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
static int log_query_statistics;
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
    std::cerr << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    std::cerr << "* PQUERY: Unable to continue [1], exiting" << std::endl;
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  if (mysql_real_connect(conn, m_conndata.addr, m_conndata.username,
  m_conndata.password, m_conndata.database, m_conndata.port, m_conndata.socket, 0) == NULL) {
    std::cerr << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    std::cerr << "* PQUERY: Unable to continue [2], exiting" << std::endl;
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  std::cout << "- PQuery v" << PQVERSION << " compiled with " << FORK << "-" << mysql_get_client_info() << std::endl;
  std::cout << "- Connected to " << mysql_get_host_info(conn) << "..." << std::endl;
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
  std::cout << "- Connected server version: " << server_version << std::endl;
  if (result != NULL) {
    mysql_free_result(result);
  }
  mysql_close(conn);
  if(test_connection){
    std::cout << "- Ending test run" << std::endl;
    mysql_library_end();
    exit(0);
  }
}

void
executor(int number, const vector<string>& qlist) {

  int failed_queries = 0;
  int total_queries = 0;
  int max_con_failures = 250;                     /* Maximum consecutive failures (likely indicating crash/assert, user priveleges drop etc.) */
  int max_con_fail_count = 0;
  int res;
  std::chrono::steady_clock::time_point begin, end;
  ofstream thread_log;

  if ((log_failed_queries) || (log_all_queries) || (log_query_statistics)) {
    ostringstream os;
    os << m_conndata.logdir << "/pquery_thread-" << number << ".log";
    thread_log.open(os.str(), ios::out | ios::app);
    if(!thread_log.is_open()){
      std::cerr << "Unable to open logfile " << os.str() << ": " << strerror(errno) << std::endl;
      return;
    }
  }

  MYSQL * conn;

  conn = mysql_init(NULL);
  if (conn == NULL) {
    std::cerr << "Error " <<  mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;

    if (thread_log) {
      thread_log.close();
    }
    std::cerr << "Thread #" << number << " is exiting abnormally" << std::endl;
    return;
  }
  if (mysql_real_connect(conn, m_conndata.addr, m_conndata.username,
  m_conndata.password, m_conndata.database, m_conndata.port, m_conndata.socket, 0) == NULL) {
    std::cerr << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    mysql_close(conn);
    if (thread_log.is_open()) {
      thread_log.close();
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
      begin = std::chrono::steady_clock::now();
    }

    res = mysql_real_query(conn, qlist[query_number].c_str(), (unsigned long)strlen(qlist[query_number].c_str()));

    if(log_query_duration) {
      end = std::chrono::steady_clock::now();
    }

    if (res == 0) {                               //success
      max_con_fail_count=0;
    }
    else {
      failed_queries++;
      max_con_fail_count++;
      if (max_con_fail_count >= max_con_failures) {
        ostringstream errmsg;
        errmsg << "* Last " << max_con_fail_count << " consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.";
        std::cerr <<  errmsg.str() << std::endl;
        if (thread_log.is_open()) {
          thread_log << errmsg.str() << std::endl;
        }
        break;
      }
    }

    total_queries++;
    MYSQL_RES * result = mysql_store_result(conn);

// logging part, initial implementation, will be refactored / rewritten
    if((verbose) && (m_conndata.threads == 1)) { // print it only if 1 thread is active

      if(res == 0) {
        if( (log_all_queries) || (log_query_statistics) ){
          std::cerr << qlist[query_number] << " # NOERROR";
          if(log_query_statistics){
            std::cerr << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: " << get_affected_rows(conn);
          }
          if(log_query_duration) {
            std::cerr << " # Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()  << " µs";
          }
          std::cerr << std::endl;
        }
      } else {
        if((log_failed_queries) || (log_all_queries) || (log_query_statistics)) {
          std::cerr << qlist[query_number] << " # ERROR: " << mysql_errno(conn) << " - " <<  mysql_error(conn);
          if(log_query_statistics){
            std::cerr << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: " << get_affected_rows(conn);
          }
           if(log_query_duration) {
            std::cerr << " # Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()  << " µs";
          }
          std::cerr << std::endl;
        }
      } //if / else
    }

//
    if(thread_log.is_open()) {
      if(res == 0) {
        if((log_all_queries) || (log_query_statistics)) {
          thread_log << qlist[query_number] << " # NOERROR";
          if(log_query_statistics){
            thread_log << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: "  << get_affected_rows(conn);
          }
          if(log_query_duration) {
            thread_log << " # Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()  << " µs";
          }
          thread_log << std::endl;
        }
      }
      else {
        if((log_failed_queries) || (log_all_queries) || (log_query_statistics)) {
          thread_log << qlist[query_number] << " # ERROR: " <<  mysql_errno(conn) << " - " << mysql_error(conn);
          if(log_query_statistics){
            thread_log << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: "  << get_affected_rows(conn);
          }
          if(log_query_duration) {
            thread_log << " # Duration: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()  << " µs";
          }
          thread_log << std::endl;
        }
      }
    }
    if (result != NULL) {
      mysql_free_result(result);
    }
  }                                               //for loop

  ostringstream exitmsg;
  exitmsg.precision(2);
  exitmsg << fixed;
  exitmsg << "* SUMMARY: " << failed_queries << "/" << total_queries << " queries failed (" <<
  (total_queries-failed_queries)*100.0/total_queries << "%) were successful)";
  std::cout << exitmsg.str() << std::endl;

  if (thread_log.is_open()) {
    thread_log << exitmsg.str() << std::endl;
    thread_log.close();
  }
  mysql_close(conn);
  mysql_thread_end();
}

int
main(int argc, char* argv[]) {

  std::ios_base::sync_with_stdio(false);
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
      {"logdir", required_argument, 0, 'l'},
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
      {"log-query-statistics", no_argument, &log_query_statistics, 1},
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
        std::cout << "Database: " << optarg << std::endl;
        memcpy(m_conndata.database, optarg, strlen(optarg) + 1);
        break;
      case 'a':
        std::cout << "Address: " << optarg << std::endl;
        memcpy(m_conndata.addr, optarg, strlen(optarg) + 1);
        break;
      case 'i':
        std::cout << "Infile: " << optarg << std::endl;
        memcpy(m_conndata.infile, optarg, strlen(optarg) + 1);
        break;
      case 'l':
        std::cout << "Logdir: " << optarg << std::endl;
        memcpy(m_conndata.logdir, optarg, strlen(optarg) + 1);
        break;
      case 's':
        std::cout << "Socket: " << optarg << std::endl;
        memcpy(m_conndata.socket, optarg, strlen(optarg) + 1);
        break;
      case 'p':
        std::cout << "Port: " << optarg << std::endl;
        m_conndata.port = atoi(optarg);
        break;
      case 'u':
        std::cout << "User: " << optarg << std::endl;
        memcpy(m_conndata.username, optarg, strlen(optarg) + 1);
        break;
      case 'P':
        std::cout << "Password: " << optarg << std::endl;
        memcpy(m_conndata.password, optarg, strlen(optarg) + 1);
        break;
      case 't':
        std::cout << "Starting with " << optarg << " thread(s)" << std::endl;
        m_conndata.threads = atoi(optarg);
        break;
      case 'q':
        std::cout << "Query limit per thread: " << optarg << std::endl;
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
    std::cerr << "Unable to open SQL file " << m_conndata.infile << ": " << strerror(errno) << std::endl;
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
