#include <iostream>
#include <hCommon.hpp>
#include <cMysqlWorker.hpp>
#include <cMysqlDatabase.hpp>

MysqlWorker::MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if (mysql_library_init(0, NULL, NULL)) {
    throw std::runtime_error("=> Could not initialize MySQL client library");
    }
  }


MysqlWorker::~MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mysql_library_end();
  }


bool
MysqlWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  std::shared_ptr<MysqlDatabase> mysqlDB = std::make_shared<MysqlDatabase>();

  if (!mysqlDB->init()) {
    wLogger->addRecordToLog("=> Unable to init, MySQL error " + mysqlDB->getErrorString());
    return false;
    }

  if(!mysqlDB->connect(mParams)) {
    wLogger->addRecordToLog("=> Connection error " + mysqlDB->getErrorString());
    return false;
    }

  wLogger->addRecordToLog("-> Successfully connected to " + mysqlDB->getHostInfo());
  wLogger->addRecordToLog("-> Server version: " + mysqlDB->getServerVersion());
  return true;

  }


std::shared_ptr<Database>
MysqlWorker::createDbInstance() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::shared_ptr<Database> mysqlDB = std::make_shared<MysqlDatabase>();
  return mysqlDB;
  }


void
MysqlWorker::endDbThread() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mysql_thread_end();
  }


//     do {
//       MYSQL_RES * result = mysql_use_result(conn);

//       if(mParams.log_client_output) {
//         std::string out;

//         if(result != NULL) {
//           MYSQL_ROW row;
//           unsigned int i, num_fields;

//           num_fields = mysql_num_fields(result);

//           while ((row = mysql_fetch_row(result))) {

//             for(i = 0; i < num_fields; i++) {

//               if (row[i]) {
//                 if(strlen(row[i]) == 0) {
//                   outputLogger->addPartialRecord("EMPTY#");
//                   }
//                 else {
//                   outputLogger->addPartialRecord(row[i]);
//                   outputLogger->addPartialRecord("#");
//                   }
//                 }
//               else {
//                 outputLogger->addPartialRecord("#NO DATA#");
//                 }
//               }
//             if(mParams.log_query_numbers) {
//               outputLogger->addPartialRecord(std::to_string(query_number+1));
//               }
//             outputLogger->addPartialRecord("\n");
//             }                                     //while
//           }                                       // if result != Null
//         }

// //

//       if(res == 0) {
//         if((mParams.log_all_queries) || (mParams.log_query_statistics)) {

//           threadLogger->addPartialRecord((*queryList)[query_number]);
//           threadLogger->addPartialRecord("#NOERROR");
//           if(mParams.log_query_statistics) {
//             threadLogger->addPartialRecord("#WARNINGS: " + std::to_string(mysql_warning_count(conn)) + "#CHANGED: " + std::to_string(getAffectedRows(conn)));
//             }
//           if(mParams.log_query_duration) {
//             threadLogger->addPartialRecord("#Duration: " + std::to_string(std::chrono::duration<double>(end - begin).count() * 1000) + " ms");
//             }
//           if(mParams.log_query_numbers) {
//             threadLogger->addPartialRecord("#" + std::to_string(query_number+1));
//             }
//           threadLogger->addPartialRecord("\n");
//           }
//         }
//       else {
//         if((mParams.log_failed_queries) || (mParams.log_all_queries) || (mParams.log_query_statistics)) {

//           threadLogger->addPartialRecord((*queryList)[query_number] +
//             "#ERROR: " + std::to_string(mysql_errno(conn)) + " - " + mysql_error(conn));
//           if(mParams.log_query_statistics) {
//             threadLogger->addPartialRecord("#WARNINGS: " + std::to_string(mysql_warning_count(conn)) + "#CHANGED: "  + std::to_string(getAffectedRows(conn)));
//             }
//           if(mParams.log_query_duration) {
//             threadLogger->addPartialRecord("#Duration: " + std::to_string(std::chrono::duration<double>(end - begin).count() * 1000) + " ms");
//             }
//           if(mParams.log_query_numbers) {
//             threadLogger->addPartialRecord("#" + std::to_string(query_number+1));
//             }
//           threadLogger->addPartialRecord("\n");
//           }
//         }

//       if (result != NULL) {
//         mysql_free_result(result);
//         }
//       }  while (mysql_next_result(conn) == 0) ;   // while
