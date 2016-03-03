#include "node.hpp"
#include <iostream>

Node::Node(){
  setDefaults();
}

Node::~Node(){

}

void
Node::setDefaults(){
  myName.clear();
  address.clear();
  user.clear();
  password.clear();
  port = 3306;
  socket = "/tmp/my.sock";
  database = "test";
  infile = "pquery.sql";
  logdir = "/tmp";
  threads = 10;
  queries_per_thread = 10000;
  verbose = false;
  debug = false;
  log_all_queries = false;
  log_failed_queries = false;
  log_query_statistics = false;
  log_query_duration = false;
  shuffle = true;
}

void
Node::StartWork(std::string secName){
  std::cout << "Connecting to " << secName << "..." << std::endl;
}
