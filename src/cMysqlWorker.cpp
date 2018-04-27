#include <iostream>
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


int
MysqlWorker::executeTests(struct workerParams& params) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(!params.myName.empty()) {
    return 0;
    }
  return 0;
  }
