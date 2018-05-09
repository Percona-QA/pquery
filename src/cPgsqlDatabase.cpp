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
PgsqlDatabase::init() {
  return true;
  }


bool
PgsqlDatabase::connect(workerParams& mParams) {
  return true;
  }


bool
PgsqlDatabase::performRealQuery(std::string query) {
  return true;
  }


std::uint32_t
PgsqlDatabase::getWarningsCount() {
  return 0;
  }


std::string
PgsqlDatabase::getServerVersion() {
  std::string server_version;
  return server_version;

  }


std::string
PgsqlDatabase::getErrorString() {
  std::string error_string;
  return error_string;
  }


std::string
PgsqlDatabase::getHostInfo() {
  std::string host_info;
  return host_info;
  }


inline std::uint64_t
PgsqlDatabase::getAffectedRows() {
  return 0;
  }
