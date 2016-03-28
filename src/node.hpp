#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <thread>
#include <INIReader.h>
#include <my_global.h>
#include <mysql.h>

/*
It represents standalone MySQL server or MySQL node in cluster (PXC) setup
*/

class Node {
  public:
    Node();
   ~Node();
   void setName(std::string name){myName = name;}
   void startWork(std::string);
 private:
  // declaration for worker thread function
  void workerThread(int);
  inline unsigned long long getAffectedRows(MYSQL*);
  void tryConnect();
  bool createGeneralLog();
  void readSettings(std::string);

  INIReader * reader;
  std::vector<std::thread> workers;
  std::vector<std::string> * querylist;

  std::ofstream general_log;
  std::string myName;
  std::string address;
  std::string socket;
  std::string username;
  std::string password;
  std::string database;
  std::string infile;
  std::string logdir;
  short port;
  short threads;
  unsigned long queries_per_thread;
  bool verbose;
  bool debug;
  bool log_all_queries;
  bool log_failed_queries;
  bool log_query_statistics;
  bool log_query_duration;
  bool log_client_output;
  bool shuffle;

};


#endif
