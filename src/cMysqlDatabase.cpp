#ifdef DEBUG
#include <iostream>
#endif

#include <cstring>
#include <cMysqlDatabase.hpp>

MysqlDatabase::MysqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  conn = NULL;
  }


MysqlDatabase::~MysqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(conn != NULL) { mysql_close(conn); }
  }


inline std::uint64_t
MysqlDatabase::getAffectedRows() {
  if (mysql_affected_rows(conn) == ~(unsigned long long) 0) {
    return 0LL;
    }
  return mysql_affected_rows(conn);
  }


bool
MysqlDatabase::connect(struct workerParams& dbParams) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  conn = mysql_init(NULL);
  if (conn == NULL) { return false; }
  if (mysql_real_connect(conn, dbParams.address.c_str(), dbParams.username.c_str(),
    dbParams.password.c_str(), dbParams.database.c_str(), dbParams.port, dbParams.socket.c_str(), 0) == NULL){ return false; }
    return true;
  }


bool
MysqlDatabase::performRealQuery(std::string query) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  int res;
  res = mysql_real_query(conn, query.c_str(), (unsigned long)query.length());
  return (res == 0);
  }


std::uint32_t
MysqlDatabase::getWarningsCount() {
  return mysql_warning_count(conn);
  }


void
MysqlDatabase::processQueryOutput() {
  queryResult.clear();
  do {
    MYSQL_RES * result = mysql_use_result(conn);
    if(result == NULL) { return; }
    MYSQL_ROW row;
    std::uint32_t i, num_fields;
    num_fields = mysql_num_fields(result);
    while ((row = mysql_fetch_row(result))) {
      for(i = 0; i < num_fields; i++) {
        if (row[i]) {
          if(strlen(row[i]) == 0) {
            queryResult = "EMPTY";
            }
          else {
            queryResult = (row[i]);
            }
          }
        else {
          queryResult = "NO DATA";
          }
        }
      }
    mysql_free_result(result);
    }  while (mysql_next_result(conn) == 0) ;     // do-while
  }


std::string
MysqlDatabase::getHostInfo() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  return mysql_get_host_info(conn);
  }


std::string
MysqlDatabase::getErrorString() {
  return (std::to_string(mysql_errno(conn)) + ": " + mysql_error(conn));
  }


std::string
MysqlDatabase::getServerVersion() {
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


void
MysqlDatabase::cleanupResult() {

  }
