#include "node.hpp"
#include <iostream>
#include <cerrno>

Node::Node(){
  std::ios_base::sync_with_stdio(false);
}

Node::~Node(){
  if (reader){
    delete reader;
  }
  if(general_log){
    general_log.close();
  }
}

bool
Node::createGeneralLog(){
  std::string logName;
  logName = logdir + "/" + myName + "_general" + ".log";
  general_log.open(logName, std::ios::out | std::ios::trunc);
  if (!general_log.is_open()){
    std::cout << "Unable to open log file " << logName << ": " << strerror(errno) << std::endl;
    return false;
  }
  return true;
}

void
Node::readSettings(std::string secName){
  address = reader->Get(secName, "address", "");
  username = reader->Get(secName, "user", "test");
  password = reader->Get(secName, "password", "test");
  socket = reader->Get(secName, "socket", "/tmp/my.sock");
  database = reader->Get(secName, "database", "test");
  infile = reader->Get(secName, "infile", "pquery.sql");
  logdir = reader->Get(secName, "logdir", "/tmp");

  port = reader->GetInteger(secName, "port", 3306);
  threads = reader->GetInteger(secName, "threads", 10);
  queries_per_thread = reader->GetInteger(secName, "queries_per_thread", 10000);

  verbose = reader->GetBoolean(secName, "verbose", false);
  debug = reader->GetBoolean(secName, "debug", false);
  log_all_queries = reader->GetBoolean(secName, "log_all_queries", false);
  log_failed_queries = reader->GetBoolean(secName, "log_failed_queries", false);
  log_query_statistics = reader->GetBoolean(secName, "log_query_statistics",  false);
  log_query_duration = reader->GetBoolean(secName, "log_query_duration", false);
  shuffle = reader->GetBoolean(secName, "shuffle", true);
}

void
Node::startWork(std::string confFile){
  reader = new INIReader(confFile);
  if (reader->ParseError() < 0) {
    std::cout << "Can't load " << confFile << std::endl;
    exit(1);
  }
  readSettings(myName);
  if(!createGeneralLog()){
    std::cerr << "Exiting..." << std::endl;
    exit(2);
  }
  std::cout << "Connecting to " << myName << "..." << std::endl;
  general_log << "Connecting to " << myName << "..." << std::endl;
}
