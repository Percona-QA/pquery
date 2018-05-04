#include <iostream>
#include <hCommon.hpp>
#include <cMysqlWorker.hpp>

MysqlWorker::MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  }


MysqlWorker::~MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mysql_library_end();
  }


inline unsigned long long
MysqlWorker::getAffectedRows(MYSQL * connection) {
  if (mysql_affected_rows(connection) == ~(unsigned long long) 0) {
    return 0LL;
    }
  return mysql_affected_rows(connection);
  }


std::string
MysqlWorker::getServerVersion(MYSQL* conn) {
  MYSQL_RES* result = NULL;
  std::string server_version;

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

  if (result != NULL) {
    mysql_free_result(result);
    }
  return server_version;
  }


bool
MysqlWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  MYSQL* conn;
  conn = mysql_init(NULL);
  if (conn == NULL) {
    wLogger->addRecordToLog("=> Unable to init, MySQL error " + std::to_string(mysql_errno(conn)) + ": " + mysql_error(conn));
    mysql_close(conn);
    return false;
    }
  if (mysql_real_connect(conn, mParams.address.c_str(), mParams.username.c_str(),
  mParams.password.c_str(), mParams.database.c_str(), mParams.port, mParams.socket.c_str(), 0) == NULL) {
    wLogger->addRecordToLog("=> Connection error " + std::to_string(mysql_errno(conn)) + ": " + mysql_error(conn));
    mysql_close(conn);
    return false;
    }
  wLogger->addRecordToLog("-> Successfully connected to " + std::string(mysql_get_host_info(conn)));
  std::string server_version = getServerVersion(conn);
  wLogger->addRecordToLog("-> Server version: " + server_version);

  mysql_close(conn);
  return true;
  }


bool
MysqlWorker::executeTests(struct workerParams& params) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(!params.myName.empty()) {
    return true;
    }
  return true;
  }


void
MysqlWorker::workerThread(int number) {
//TODO
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << number << std::endl;
#endif
// TODO
  int failed_queries = 0;
  int total_queries = 0;
  int max_con_fail_count = 0;
  int max_con_failures = 250;
  std::shared_ptr<Logger> outputLogger;
  std::shared_ptr<Logger> threadLogger;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dis(0, queryList->size() - 1);
  int res;
  MYSQL * conn;

  if(mParams.log_client_output) {
    outputLogger = std::make_shared<Logger>();
    outputLogger->initLogFile(mParams.logdir + "/" + mParams.myName + "_thread-" + std::to_string(number) + ".out");
    }

  if ((mParams.log_failed_queries) || (mParams.log_all_queries) || (mParams.log_query_statistics) || (mParams.log_succeeded_queries)) {
    threadLogger = std::make_shared<Logger>();
    threadLogger->initLogFile(mParams.logdir + "/" + mParams.myName + "_thread-" + std::to_string(number) + ".sql");
    }
  if(mParams.log_query_duration) { threadLogger->setPrecision(3); }

  conn = mysql_init(NULL);
  if (conn == NULL) {
    threadLogger->addRecordToLog("Error " + std::to_string(mysql_errno(conn)) + ": " + mysql_error(conn) );
    wLogger->addRecordToLog("==> Thread #" + std::to_string(number) + " is exiting abnormally, unable to init MySQL connection");
    return;
    }

#ifdef MAXPACKET
  if (mParams.maxpacket != MAX_PACKET_DEFAULT) {
    mysql_options(conn, MYSQL_OPT_MAX_ALLOWED_PACKET, &mParams.maxpacket);
    }
#endif

  if (mysql_real_connect(conn, mParams.address.c_str(), mParams.username.c_str(),
  mParams.password.c_str(), mParams.database.c_str(), mParams.port, mParams.socket.c_str(), 0) == NULL) {
    threadLogger->addRecordToLog("Error " + std::to_string(mysql_errno(conn)) + ": " + mysql_error(conn));
    mysql_close(conn);
    mysql_thread_end();
    return;
    }

  unsigned long i;
  for (i=0; i<mParams.queries_per_thread; i++) {

    unsigned long query_number;
// selecting query #, depends on random or sequential execution

    if(!mParams.shuffle) {
      query_number = i;
      }
    else {
      query_number = dis(gen);
      }

    if(mParams.log_query_duration) {
      begin = std::chrono::steady_clock::now();
      }

    res = mysql_real_query(conn, (*queryList)[query_number].c_str(),
      (unsigned long)strlen((*queryList)[query_number].c_str()));

    if(mParams.log_query_duration) {
      end = std::chrono::steady_clock::now();
      }

    total_queries++;

    if (res == 0) {                               //success
      max_con_fail_count=0;
      }
    else {
      failed_queries++;
      max_con_fail_count++;
      if (max_con_fail_count >= max_con_failures) {
        threadLogger->addRecordToLog("* Last " + std::to_string(max_con_fail_count) +
          " consecutive queries all failed. Likely crash/assert, user privileges drop, or similar. Ending run.");
        return;
        }
      }

    do {
      MYSQL_RES * result = mysql_use_result(conn);

      if(mParams.log_client_output) {
        std::string out;

        if(result != NULL) {
          MYSQL_ROW row;
          unsigned int i, num_fields;

          num_fields = mysql_num_fields(result);

          while ((row = mysql_fetch_row(result))) {

            for(i = 0; i < num_fields; i++) {

              if (row[i]) {
                if(strlen(row[i]) == 0) {
                  outputLogger->addPartialRecord("EMPTY#");
                  }
                else {
                  outputLogger->addPartialRecord(row[i]);
                  outputLogger->addPartialRecord("#");
                  }
                }
              else {
                outputLogger->addPartialRecord("#NO DATA#");
                }
              }
            if(mParams.log_query_numbers) {
              outputLogger->addPartialRecord(std::to_string(query_number+1));
              }
            outputLogger->addPartialRecord("\n");
            }                                     //while
          }                                       // if result != Null
        }

//

      if(res == 0) {
        if((mParams.log_all_queries) || (mParams.log_query_statistics)) {

          threadLogger->addPartialRecord((*queryList)[query_number]);
          threadLogger->addPartialRecord("#NOERROR");
          if(mParams.log_query_statistics) {
            threadLogger->addPartialRecord("#WARNINGS: " + std::to_string(mysql_warning_count(conn)) + "#CHANGED: " + std::to_string(getAffectedRows(conn)));
            }
          if(mParams.log_query_duration) {
            threadLogger->addPartialRecord("#Duration: " + std::to_string(std::chrono::duration<double>(end - begin).count() * 1000) + " ms");
            }
          if(mParams.log_query_numbers) {
            threadLogger->addPartialRecord("#" + std::to_string(query_number+1));
            }
          threadLogger->addPartialRecord("\n");
          }
        }
      else {
        if((mParams.log_failed_queries) || (mParams.log_all_queries) || (mParams.log_query_statistics)) {

          threadLogger->addPartialRecord((*queryList)[query_number] +
            "#ERROR: " + std::to_string(mysql_errno(conn)) + " - " + mysql_error(conn));
          if(mParams.log_query_statistics) {
            threadLogger->addPartialRecord("#WARNINGS: " + std::to_string(mysql_warning_count(conn)) + "#CHANGED: "  + std::to_string(getAffectedRows(conn)));
            }
          if(mParams.log_query_duration) {
            threadLogger->addPartialRecord("#Duration: " + std::to_string(std::chrono::duration<double>(end - begin).count() * 1000) + " ms");
            }
          if(mParams.log_query_numbers) {
            threadLogger->addPartialRecord("#" + std::to_string(query_number+1));
            }
          threadLogger->addPartialRecord("\n");
          }
        }

      if (result != NULL) {
        mysql_free_result(result);
        }
      }  while (mysql_next_result(conn) == 0) ;   // while

    }                                             // for

  mysql_close(conn);
  mysql_thread_end();
  performed_queries_total += total_queries;
  failed_queries_total += failed_queries;

  }                                               // thread
