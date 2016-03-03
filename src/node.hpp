#ifndef __NODE_HPP__
#define __NODE_HPP__

#include <string>
#include <INIReader.h>

/*
It represents standalone MySQL server or MySQL node in cluster (PXC) setup
*/

class Node {
  public:
    Node();
   ~Node();
   void StartWork(std::string);
 private:
  void setDefaults();
  std::string myName;
  std::string address;
  std::string socket;
  std::string user;
  std::string password;
  std::string database;
  std::string infile;
  std::string logdir;
  short port;
  short threads;
  unsigned queries_per_thread;
  bool verbose;
  bool debug;
  bool log_all_queries;
  bool log_failed_queries;
  bool log_query_statistics;
  bool log_query_duration;
  bool shuffle;

};


#endif
