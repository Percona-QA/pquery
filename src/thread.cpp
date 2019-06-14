#include "common.hpp"
#include "node.hpp"
#include "random_test.hpp"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <random>

inline unsigned long long Node::getAffectedRows(MYSQL *connection) {
  if (mysql_affected_rows(connection) == ~(unsigned long long)0) {
    return 0LL;
  }
  return mysql_affected_rows(connection);
}

void Node::random_Generated_Load(int number) {
  number++;
  return;
}
void Node::workerThread(int number) {

  int failed_queries = 0;
  int total_queries = 0;
  int max_con_failures = 250; /* Maximum consecutive failures (likely indicating
                                 crash/assert, user priveleges drop etc.) */
  int max_con_fail_count = 0;
  int res;

  std::chrono::steady_clock::time_point begin, end;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, querylist->size() - 1);
  std::ofstream thread_log;
  std::ofstream client_log;

  if (myParams.log_client_output) {
    std::ostringstream cl;
    cl << myParams.logdir << "/" << myParams.myName << "_thread-" << number
       << ".out";
    client_log.open(cl.str(), std::ios::out | std::ios::trunc);
    if (!client_log.is_open()) {
      general_log << "Unable to open logfile for client output " << cl.str()
                  << ": " << std::strerror(errno) << std::endl;
      return;
    }
  }

  if ((myParams.log_failed_queries) || (myParams.log_all_queries) ||
      (myParams.log_query_statistics) || (myParams.log_succeeded_queries)) {
    std::ostringstream os;
    os << myParams.logdir << "/" << myParams.myName << "_thread-" << number
       << ".sql";
    thread_log.open(os.str(), std::ios::out | std::ios::trunc);
    if (!thread_log.is_open()) {
      general_log << "Unable to open thread logfile " << os.str() << ": "
                  << std::strerror(errno) << std::endl;
      return;
    }
    if (myParams.log_query_duration) {
      thread_log.precision(3);
      thread_log << std::fixed;
      std::cerr.precision(3);
      std::cerr << std::fixed;
      std::cout.precision(3);
      std::cout << std::fixed;
    }
  }

  MYSQL *conn;

  conn = mysql_init(NULL);
  if (conn == NULL) {
    thread_log << "Error " << mysql_errno(conn) << ": " << mysql_error(conn)
               << std::endl;

    if (thread_log) {
      thread_log.close();
    }
    general_log << ": Thread #" << number << " is exiting abnormally"
                << std::endl;
    return;
  }
#ifdef MAXPACKET
  if (myParams.maxpacket != MAX_PACKET_DEFAULT) {
    mysql_options(conn, MYSQL_OPT_MAX_ALLOWED_PACKET, &myParams.maxpacket);
  }
#endif
  if (mysql_real_connect(conn, myParams.address.c_str(),
                         myParams.username.c_str(), myParams.password.c_str(),
                         myParams.database.c_str(), myParams.port,
                         myParams.socket.c_str(), 0) == NULL) {
    thread_log << "Error " << mysql_errno(conn) << ": " << mysql_error(conn)
               << std::endl;
    mysql_close(conn);
    if (thread_log.is_open()) {
      thread_log.close();
    }
    mysql_thread_end();
    return;
  }

  Thd1 *THD = new Thd1(thread_log, conn, tables);

  if (number == 0 ) {
    run_default_load(THD);
    default_load = true;
  }

  while (!default_load) {
    std::chrono::seconds dura(3);
    std::this_thread::sleep_for(dura);
    thread_log << "waiting for defalut load to finish" << std::endl;
  }

  // run_some_query(THD);
  delete THD;

  unsigned long i;
  for (i = 0; i < myParams.queries_per_thread; i++) {

    unsigned long query_number;
    // selecting query #, depends on random or sequential execution
    if (!myParams.shuffle) {
      query_number = i;
    } else {
      query_number = dis(gen);
    }

    // perform the query and getting the result

    if (myParams.log_query_duration) {
      begin = std::chrono::steady_clock::now();
    }

    res = mysql_real_query(
        conn, (*querylist)[query_number].c_str(),
        (unsigned long)strlen((*querylist)[query_number].c_str()));

    if (myParams.log_query_duration) {
      end = std::chrono::steady_clock::now();
    }

    if (res == 0) { // success
      max_con_fail_count = 0;
    } else {
      failed_queries++;
      max_con_fail_count++;
      if (max_con_fail_count >= max_con_failures) {
        std::ostringstream errmsg;
        errmsg << "* Last " << max_con_fail_count
               << " consecutive queries all failed. Likely crash/assert, user "
                  "privileges drop, or similar. Ending run.";
        std::cerr << errmsg.str() << std::endl;
        if (thread_log.is_open()) {
          thread_log << errmsg.str() << std::endl;
        }
        break;
      }
    }

    total_queries++;
    do {
      MYSQL_RES *result = mysql_use_result(conn);
      if (myParams.log_client_output) {
        if (result != NULL) {
          MYSQL_ROW row;
          unsigned int i, num_fields;

          num_fields = mysql_num_fields(result);
          while ((row = mysql_fetch_row(result))) {
            for (i = 0; i < num_fields; i++) {
              if (row[i]) {
                if (strlen(row[i]) == 0) {
                  client_log << "EMPTY"
                             << "#";
                } else {
                  client_log << row[i] << "#";
                }
              } else {
                client_log << "#NO DATA"
                           << "#";
              }
            }
            if (myParams.log_query_numbers) {
              client_log << query_number + 1;
            }
            client_log << '\n';
          }
        }
      }

      //
      if (thread_log.is_open()) {
        if (res == 0) {
          if ((myParams.log_all_queries) || (myParams.log_query_statistics)) {

            thread_log << (*querylist)[query_number] << "#NOERROR";
            if (myParams.log_query_statistics) {
              thread_log << "#WARNINGS: " << mysql_warning_count(conn)
                         << "#CHANGED: " << getAffectedRows(conn);
            }
            if (myParams.log_query_duration) {
              thread_log << "#Duration: "
                         << std::chrono::duration<double>(end - begin).count() *
                                1000
                         << " ms";
            }
            if (myParams.log_query_numbers) {
              thread_log << "#" << query_number + 1;
            }
            thread_log << "\n";
          }
        } else {
          if ((myParams.log_failed_queries) || (myParams.log_all_queries) ||
              (myParams.log_query_statistics)) {

            thread_log << (*querylist)[query_number]
                       << "#ERROR: " << mysql_errno(conn) << " - "
                       << mysql_error(conn);
            if (myParams.log_query_statistics) {
              thread_log << "#WARNINGS: " << mysql_warning_count(conn)
                         << "#CHANGED: " << getAffectedRows(conn);
            }
            if (myParams.log_query_duration) {
              thread_log << "#Duration: "
                         << std::chrono::duration<double>(end - begin).count() *
                                1000
                         << " ms";
            }
            if (myParams.log_query_numbers) {
              thread_log << "#" << query_number + 1;
            }
            thread_log << "\n";
          }
        }
      }
      if (result != NULL) {
        mysql_free_result(result);
      }
    } while (mysql_next_result(conn) == 0); // while
  }                                         // for loop

  if (thread_log.is_open()) {
    thread_log.close();
  }

  if (client_log.is_open()) {
    client_log.close();
  }

  mysql_close(conn);
  mysql_thread_end();
  performed_queries_total += total_queries;
  failed_queries_total += failed_queries;
}
