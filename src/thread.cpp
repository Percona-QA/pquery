#include "node.hpp"

#include <chrono>
#include <random>
#include <algorithm>
#include <cstring>



void
Node::workerThread(int number) {

  int failed_queries = 0;
  int total_queries = 0;
  int max_con_failures = 250;                     /* Maximum consecutive failures (likely indicating crash/assert, user priveleges drop etc.) */
  int max_con_fail_count = 0;
  int res;

  std::chrono::steady_clock::time_point begin, end;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, querylist->size() - 1);
  std::ofstream thread_log;
  std::ofstream client_log;

  if(log_client_output){
    std::ostringstream cl;
    cl << logdir << "/" << myName << "_thread-" << number << ".out";
    client_log.open(cl.str(), std::ios::out | std::ios::app);
    if(!client_log.is_open()) {
      general_log << "Unable to open logfile for client output " << cl.str() << ": " << std::strerror(errno) << std::endl;
    return;
    }
  }

  if ((log_failed_queries) || (log_all_queries) || (log_query_statistics)) {
    std::ostringstream os;
    os << logdir << "/" << myName << "_thread-" << number << ".sql";
    thread_log.open(os.str(), std::ios::out | std::ios::app);
    if(!thread_log.is_open()) {
      general_log << "Unable to open thread logfile " << os.str() << ": " << std::strerror(errno) << std::endl;
      return;
    }
    if(log_query_duration) {
      thread_log.precision(3);
      thread_log << std::fixed;
      std::cerr.precision(3);
      std::cerr << std::fixed;
      std::cout.precision(3);
      std::cout << std::fixed;
    }
  }

  MYSQL * conn;

  conn = mysql_init(NULL);
  if (conn == NULL) {
    thread_log << "Error " <<  mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;

    if (thread_log) {
      thread_log.close();
    }
    general_log << ": Thread #" << number << " is exiting abnormally" << std::endl;
    return;
  }
  if (mysql_real_connect(conn, address.c_str(), username.c_str(),
  password.c_str(), database.c_str(), port, socket.c_str(), 0) == NULL) {
    thread_log << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    mysql_close(conn);
    if (thread_log.is_open()) {
      thread_log.close();
    }
    mysql_thread_end();
    return;
  }

  unsigned long i;
  for (i=0; i<queries_per_thread; i++) {

    unsigned long query_number;

    if(!shuffle) {
      query_number = i;
    }
    else {
      query_number = dis(gen);
    }

// perform the query and getting the result

    if(log_query_duration) {
      begin = std::chrono::steady_clock::now();
    }

    res = mysql_real_query(conn, (*querylist)[query_number].c_str(), (unsigned long)strlen((*querylist)[query_number].c_str()));

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
        std::ostringstream errmsg;
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
    if(log_client_output){
      if(result != NULL){
        MYSQL_ROW row;
        unsigned int i, num_fields;

        num_fields = mysql_num_fields(result);
        while ((row = mysql_fetch_row(result))){
          for(i = 0; i < num_fields; i++){
            if (row[i]){
              client_log << row[i] << " | ";
            }else{
              client_log << "NULL" << " | ";
            }
          }
          client_log << '\n';
        }
      }
    }

//
    if(thread_log.is_open()) {
      if(res == 0) {
        if((log_all_queries) || (log_query_statistics)) {
          thread_log << (*querylist)[query_number] << " # NOERROR";
          if(log_query_statistics) {
            thread_log << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: "  << getAffectedRows(conn);
          }
          if(log_query_duration) {
            thread_log << " # Duration: " << std::chrono::duration<double>(end - begin).count() * 1000 << " ms";
          }
          thread_log << "\n";
        }
      }
      else {
        if((log_failed_queries) || (log_all_queries) || (log_query_statistics)) {
          thread_log << (*querylist)[query_number] << " # ERROR: " <<  mysql_errno(conn) << " - " << mysql_error(conn);
          if(log_query_statistics) {
            thread_log << " # WARNINGS: " << mysql_warning_count(conn) << " # CHANGED: "  << getAffectedRows(conn);
          }
          if(log_query_duration) {
            thread_log << " # Duration: " << std::chrono::duration<double>(end - begin).count() * 1000 << " ms";
          }
          thread_log << "\n";
        }
      }
    }
    if (result != NULL) {
      mysql_free_result(result);
    }
  }                                               //for loop

  std::ostringstream exitmsg;
  exitmsg.precision(2);
  exitmsg << std::fixed;
  exitmsg << "* SUMMARY: " << failed_queries << "/" << total_queries << " queries failed (" <<
    (total_queries-failed_queries)*100.0/total_queries << "%) were successful)";
  // std::cout << exitmsg.str() << std::endl;

  if (thread_log.is_open()) {
    thread_log << exitmsg.str() << std::endl;
    thread_log.close();
  }
  if(client_log.is_open()) {
    client_log.close();
  }

  mysql_close(conn);
  mysql_thread_end();
}

inline unsigned long long
Node::getAffectedRows(MYSQL * connection) {
  if (mysql_affected_rows(connection) == ~(ulonglong) 0) {
    return 0LL;
  }
  return mysql_affected_rows(connection);
}
