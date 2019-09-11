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

void Node::workerThread(int number) {

  std::ofstream thread_log;
  std::ofstream client_log;
  if (options->at(Option::LOG_CLIENT_OUTPUT)->getBool()) {
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

  if (options->at(Option::LOG_FAILED_QUERIES)->getBool() ||
      options->at(Option::LOG_ALL_QUERIES)->getBool() ||
      options->at(Option::LOG_QUERY_STATISTICS)->getBool() ||
      options->at(Option::LOG_SUCCEDED_QUERIES)->getBool()) {
    std::ostringstream os;
    os << myParams.logdir << "/" << myParams.myName << "_thread-" << number
       << ".sql";
    thread_log.open(os.str(), std::ios::out | std::ios::trunc);
    if (!thread_log.is_open()) {
      general_log << "Unable to open thread logfile " << os.str() << ": "
                  << std::strerror(errno) << std::endl;
      return;
    }

    if (options->at(Option::LOG_QUERY_DURATION)->getBool()) {
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

  Thd1 *thd = new Thd1(number, thread_log, general_log, client_log, conn);

  /* run pquery in with dynamic generator or infile */
  if (options->at(Option::DYNAMIC_PQUERY)->getBool()) {
    static bool success = false;

    /* load metadata */
    if (Thd1::metadata_locked.try_lock()) {
      if (Thd1::metadata_loaded)
        Thd1::metadata_locked.unlock();
      else {
        success = thd->load_metadata();
        Thd1::metadata_loaded = true;
        Thd1::metadata_locked.unlock();
      }
    }

    /* wait untill metadata is finished */
    while (!Thd1::metadata_loaded) {
      std::chrono::seconds dura(3);
      std::this_thread::sleep_for(dura);
      thread_log << "waiting for metadata load to finish" << std::endl;
    }

    if (!success)
      thread_log << " initial setup failed, check logs for details "
                 << std::endl;
    else
      thd->run_some_query();

  } else {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dis(0, querylist->size() - 1);
    int max_con_failures = 250;
    for (unsigned long i = 0; i < myParams.queries_per_thread; i++) {
      unsigned long query_number;
      // selecting query #, depends on random or sequential execution
      if (options->at(Option::NO_SHUFFLE)->getBool()) {
        query_number = i;
      } else {
        query_number = dis(gen);
      }
      // perform the query and getting the result
      execute_sql((*querylist)[query_number].c_str(), thd);

      if (thd->max_con_fail_count >= max_con_failures) {
        std::ostringstream errmsg;
        errmsg << "* Last " << thd->max_con_fail_count
               << " consecutive queries all failed. Likely crash/assert, user "
                  "privileges drop, or similar. Ending run.";
        std::cerr << errmsg.str() << std::endl;
        if (thread_log.is_open()) {
          thread_log << errmsg.str() << std::endl;
        }
        break;
      }
    }
  }
  delete thd;

  if (thread_log.is_open())
    thread_log.close();

  if (client_log.is_open())
    client_log.close();

  mysql_close(conn);
  mysql_thread_end();
}
