#ifdef DEBUG
#include <iostream>
#endif

#include <cMysqlDatabase.hpp>

MysqlDatabase::MysqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  conn = NULL;
  }

inline unsigned long long
MysqlDatabase::getAffectedRows() {
  if (mysql_affected_rows(conn) == ~(unsigned long long) 0) {
    return 0LL;
    }
  return mysql_affected_rows(conn);
  }

bool
MysqlDatabase::init(){
  conn = mysql_init(NULL);
  if (conn == NULL) { return false; }
  return true;
}

bool
MysqlDatabase::connect(std::string address, std::string username, std::string password, std::string database, int port, std::string socket) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  if (mysql_real_connect(conn, address.c_str(), username.c_str(),
    password.c_str(), database.c_str(), port, socket.c_str(), 0) == NULL){ return false; }
    return true;
  }


MysqlDatabase::~MysqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(conn != NULL) { mysql_close(conn); }
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
