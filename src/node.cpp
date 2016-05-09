#include "node.hpp"
#include <iostream>
#include <cerrno>
#include <cstring>

Node::Node(){
  workers.clear();
}

Node::~Node(){
  if (reader){
    delete reader;
  }
  if(general_log){
    general_log.close();
  }
  if(querylist){
    delete querylist;
  }
}

bool
Node::createGeneralLog(){
  std::string logName;
  logName = logdir + "/" + myName + "_general" + ".log";
  general_log.open(logName, std::ios::out | std::ios::trunc);
  if (!general_log.is_open()){
    std::cout << "Unable to open log file " << logName << ": " << std::strerror(errno) << std::endl;
    return false;
  }
  return true;
}

void 
Node::setAllParams(struct workerParams& myParams){
  myName = myParams.myName;
  address = myParams.address;
  username = myParams.username;
  password = myParams.password;
  socket = myParams.socket;
  database = myParams.database;
  infile = myParams.infile;
  logdir = myParams.logdir;

  port = myParams.port;
  threads = myParams.threads;
  queries_per_thread = myParams.queries_per_thread;

  verbose = myParams.verbose;
  debug = myParams.debug;
  log_all_queries = myParams.log_all_queries;
  log_failed_queries = myParams.log_failed_queries;
  log_query_statistics = myParams.log_query_statistics;
  log_query_duration = myParams.log_query_duration;
  log_client_output = myParams.log_client_output;
  log_query_numbers = myParams.log_query_numbers;
  shuffle = myParams.shuffle;
}

void
Node::startWork(){
  
  if(!createGeneralLog()){
    std::cerr << "Exiting..." << std::endl;
    exit(2);
  }

  std::cout << "- Connecting to " << myName << " [" << address << "]..." << std::endl;
  general_log << "- Connecting to " << myName << " [" << address << "]..." << std::endl;
  tryConnect();

  std::ifstream sqlfile_in;
  sqlfile_in.open(infile);

  if (!sqlfile_in.is_open()) {
    std::cerr << "Unable to open SQL file " << infile << ": " << strerror(errno) << std::endl;
    general_log << "Unable to open SQL file " << infile << ": " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
  }
  querylist = new std::vector<std::string>;
  std::string line;

  while (getline(sqlfile_in, line)) {
    if(!line.empty()) {
      querylist->push_back(line);
    }
  }

  sqlfile_in.close();
  general_log << "- Read " << querylist->size() << " lines from " << infile << std::endl;

  /* log replaying */
  if(!shuffle) {
    threads = 1;
    queries_per_thread = querylist->size();
  }
/* END log replaying */
  workers.resize(threads);

  for (int i=0; i<threads; i++) {
    workers[i] = std::thread(&Node::workerThread, this, i);
  }

  for (int i=0; i<threads; i++) {
    workers[i].join();
  }
}

void
Node::tryConnect() {
  MYSQL * conn;
  conn = mysql_init(NULL);
  if (conn == NULL) {
    std::cerr << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    std::cerr << "* PQUERY: Unable to continue [1], exiting" << std::endl;
    general_log << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    general_log << "* PQUERY: Unable to continue [1], exiting" << std::endl;
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  if (mysql_real_connect(conn, address.c_str(), username.c_str(),
  password.c_str(), database.c_str(), port, socket.c_str(), 0) == NULL) {
    std::cerr << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    std::cerr << "* PQUERY: Unable to continue [2], exiting" << std::endl;
    general_log << "Error " << mysql_errno(conn) << ": " << mysql_error(conn) << std::endl;
    general_log << "* PQUERY: Unable to continue [2], exiting" << std::endl;
    mysql_close(conn);
    mysql_library_end();
    exit(EXIT_FAILURE);
  }
  general_log << "- PQuery v" << PQVERSION << " compiled with " << FORK << "-" << mysql_get_client_info() << std::endl;
  general_log << "- Connected to " << mysql_get_host_info(conn) << "..." << std::endl;
// getting the real server version
  MYSQL_RES *result = NULL;
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
  general_log << "- Connected server version: " << server_version << std::endl;
  if (result != NULL) {
    mysql_free_result(result);
  }
  mysql_close(conn);
  if(test_connection){
    exit(EXIT_SUCCESS);
  }
}
