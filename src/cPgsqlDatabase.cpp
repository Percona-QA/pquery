#include <iostream>
#include <sstream>
#include <cPgsqlDatabase.hpp>

PgsqlDatabase::PgsqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  conn = NULL;
  }


PgsqlDatabase::~PgsqlDatabase() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(conn != NULL) { PQfinish(conn); }
  }


void
PgsqlDatabase::processQueryOutput() {

  }


bool
PgsqlDatabase::connect(workerParams& dbParams) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::ostringstream conninfo;
  conninfo << "host=" << dbParams.address << " user=" << dbParams.username << " password=" << dbParams.password
    << " dbname=" << dbParams.database << " port=" << dbParams.port;
  conn = PQconnectdb(conninfo.str().c_str());
  if (PQstatus(conn) != CONNECTION_OK) { return false; }
  return true;
  }


bool
PgsqlDatabase::performRealQuery(std::string query) {
  PGresult *res = PQexec(conn, query.c_str());
  return (PQresultStatus(res) == PGRES_TUPLES_OK);
  }


std::uint32_t
PgsqlDatabase::getWarningsCount() {
  return 0;
  }


std::string
PgsqlDatabase::getServerVersion() {
  std::string server_version;
  PGresult *res = PQexec(conn, "SELECT VERSION()");
  if (PQresultStatus(res) == PGRES_TUPLES_OK) {
    server_version = PQgetvalue(res, 0, 0);
    }
  else {
    server_version = "PostgreSQL Server (Unknown)";
    }
  if (res != NULL) {
    PQclear(res);
    }
  return server_version;
  }


std::string
PgsqlDatabase::getErrorString() {
  std::string psql_errstring = PQerrorMessage(conn);
  std::size_t found = psql_errstring.find_first_of("\n");
  if(found == std::string::npos) {
    return psql_errstring;
    }
  return psql_errstring.substr(0, found);
  }


std::string
PgsqlDatabase::getHostInfo() {
  std::string host_info = PQhost(conn);
  host_info += " port ";
  host_info += PQport(conn);
  return host_info;
  }


inline std::uint64_t
PgsqlDatabase::getAffectedRows() {
  return 0;
  }
