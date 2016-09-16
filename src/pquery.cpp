/*
 =========================================================
 #       Created by Alexey Bychko, Percona LLC           #
 #     Expanded by Roel Van de Paar, Percona LLC         #
 =========================================================
*/

#include <unistd.h>
#include <getopt.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <cstring>
#include <cerrno>

#include <INIReader.h>
#include "pquery.hpp"
#include "node.hpp"
#include <mysql.h>

std::string confFile;
pid_t childPID, wPID;
int status;

void
set_defaults(struct workerParams& Params) {
// initialize all fields with default values
  Params.myName = "default.node.tld";
  Params.database = "test";
  Params.address = "localhost";
  Params.socket = "/tmp/socket.sock";
  Params.username = "root";
  Params.password = "";
  Params.infile = "pquery.sql";
  Params.logdir = "/tmp";
  Params.port = 3306;
  Params.threads = 1;
  Params.queries_per_thread = 10000;
  Params.verbose = false;
  Params.debug = false;
  Params.log_all_queries = false;
  Params.log_succeeded_queries = false,
  Params.log_failed_queries = false;
  Params.log_query_statistics = false;
  Params.log_query_duration = false;
  Params.log_client_output = false;
  Params.log_query_numbers = false;
  Params.shuffle = true;
  Params.test_connection = false;
  }


void
read_section_settings(struct workerParams& wParams, std::string secName, std::string confFile) {
  set_defaults(wParams);
  INIReader reader(confFile);
  wParams.myName = secName;
  wParams.address = reader.Get(secName, "address", "localhost");
  wParams.username = reader.Get(secName, "user", "test");
  wParams.password = reader.Get(secName, "password", "");
  wParams.socket = reader.Get(secName, "socket", "/tmp/my.sock");
  wParams.database = reader.Get(secName, "database", "test");

  wParams.port = reader.GetInteger(secName, "port", 3306);
  wParams.threads = reader.GetInteger(secName, "threads", 10);
  wParams.queries_per_thread = reader.GetInteger(secName, "queries-per-thread", 10000);

  wParams.verbose = reader.GetBoolean(secName, "verbose", false);
  wParams.debug = reader.GetBoolean(secName, "debug", false);
  wParams.shuffle = reader.GetBoolean(secName, "shuffle", true);
  wParams.infile = reader.Get(secName, "infile", "pquery.sql");
  wParams.logdir = reader.Get(secName, "logdir", "/tmp");
  wParams.test_connection = reader.GetBoolean(secName, "test-connection", false);

  wParams.log_all_queries = reader.GetBoolean(secName, "log-all-queries", false);
  wParams.log_succeeded_queries = reader.GetBoolean(secName, "log-succeded-queries", false);
  wParams.log_failed_queries = reader.GetBoolean(secName, "log-failed-queries", false);
  wParams.log_query_statistics = reader.GetBoolean(secName, "log-query-statistics",  false);
  wParams.log_query_duration = reader.GetBoolean(secName, "log-query-duration", false);
  wParams.log_client_output = reader.GetBoolean(secName, "log-client-output", false);
  wParams.log_query_numbers = reader.GetBoolean(secName, "log-query-numbers", false);

  }


void
create_worker(struct workerParams& Params) {
  childPID = fork();
  if(childPID < 0) {
    std::cerr << "=> Cannot fork() child process: " << strerror(errno) << std::endl;
    exit(EXIT_FAILURE);
    }
  if(childPID > 0) {
    std::cerr << "* Waiting for created worker " << childPID << std::endl;
    }
  if (childPID == 0) {
    std::shared_ptr<Node> newNode = std::make_shared<Node>();
    newNode->setAllParams(Params);
    newNode->startWork();
    return;
    }
  }


int
main(int argc, char* argv[]) {

  std::ios_base::sync_with_stdio(false);
  confFile.clear();
  static struct workerParams wParams;
  set_defaults(wParams);                          // reset all settings in struct to defaults
  int c;

  while(true) {

    static struct option long_options[] = {
// config file with all options
      {"config-file", required_argument, 0, 'c'},
// help options
      {"help", no_argument, 0, 'h'},
      {"config-help", no_argument, 0, 'I'},
      {"cli-help", no_argument, 0, 'C'},
// single-node options
      {"database", required_argument, 0, 'd'},
      {"address", required_argument, 0, 'a'},
      {"infile", required_argument, 0, 'i'},
      {"logdir", required_argument, 0, 'l'},
      {"socket", required_argument, 0, 's'},
      {"port", required_argument, 0, 'p'},
      {"user", required_argument, 0, 'u'},
      {"password", required_argument, 0, 'P'},
      {"threads", required_argument, 0, 't'},
      {"queries-per-thread", required_argument, 0, 'q'},
      {"verbose", no_argument, 0, 'v'},
      {"debug", no_argument, 0, 'E'},
      {"log-all-queries", no_argument, 0, 'A'},
      {"log-succeded-queries", no_argument, 0, 'S'},
      {"log-failed-queries", no_argument, 0, 'F'},
      {"no-shuffle", no_argument, 0, 'n'},
      {"log-query-statistics", no_argument, 0, 'L'},
      {"log-query-duration", no_argument, 0, 'D'},
      {"test-connection", no_argument, 0, 'T'},
      {"log-query-numbers", no_argument, 0, 'N'},
      {"log-client-output", no_argument, 0, 'O'},
// finally
      {0, 0, 0, 0}
      };

    int option_index = 0;

    c = getopt_long_only(argc, argv, "c:d:a:i:l:s:p:u:P:t:q:vAEFNLDTNOS", long_options, &option_index);

    if (c == -1) {
      break;
      }

    switch (c) {
      case 'h':
        show_help();
        exit(EXIT_FAILURE);
      case 'I':
        show_config_help();
        exit(EXIT_FAILURE);
      case 'C':
        show_cli_help();
        exit(EXIT_FAILURE);
      case 'c':
        std::cout << "> Config file: " << optarg << std::endl;
        confFile = optarg;
        break;
      case 'd':
        std::cout << "> Database: " << optarg << std::endl;
        wParams.database = optarg;
        break;
      case 'a':
        std::cout << "> Address: " << optarg << std::endl;
        wParams.address = optarg;
        break;
      case 'i':
        std::cout << "> Infile: " << optarg << std::endl;
        wParams.infile = optarg;
        break;
      case 'l':
        std::cout << "> Logdir: " << optarg << std::endl;
        wParams.logdir = optarg;
        break;
      case 's':
        std::cout << "> Socket: " << optarg << std::endl;
        wParams.socket = optarg;
        break;
      case 'p':
        std::cout << "> Port: " << optarg << std::endl;
        wParams.port = atoi(optarg);
        break;
      case 'u':
        std::cout << "> Username: " << optarg << std::endl;
        wParams.username = optarg;
        break;
      case 'P':
        std::cout << "> Password: " << optarg << std::endl;
        wParams.password = optarg;
        break;
      case 't':
        std::cout << "> Threads: " << optarg << std::endl;
        wParams.threads = atoi(optarg);
        break;
      case 'q':
        std::cout << "> Queries per thread: " << optarg << std::endl;
        wParams.queries_per_thread = atoi(optarg);
        break;
      case 'v':
        std::cout << "> Verbose mode: ON" << std::endl;
        wParams.verbose = true;
        break;
      case 'A':
        std::cout << "> Log all queries: ON" << std::endl;
        wParams.log_all_queries = true;
        break;
      case 'S':
        std::cout << "> Log succeded queries: ON" << std::endl;
        wParams.log_succeeded_queries = true;
        break;
      case 'F':
        std::cout << "> Log failed queries: ON" << std::endl;
        wParams.log_failed_queries = true;
        break;
      case 'E':
        std::cout << "> Debug mode: ON" << std::endl;
        wParams.debug = true;
        break;
      case 'n':
        std::cout << "> Shuffle mode: OFF" << std::endl;
        wParams.shuffle = false;
        break;
      case 'L':
        std::cout << "> Log query statistics: ON" << std::endl;
        wParams.log_query_statistics = true;
        break;
      case 'D':
        std::cout << "> Log query duration: ON" << std::endl;
        wParams.log_query_duration = true;
        break;
      case 'T':
        std::cout << "> Test connection and exit: ON" << std::endl;
        wParams.test_connection = true;
        break;
      case 'N':
        std::cout << "> Log query numbers: ON" << std::endl;
        wParams.log_query_numbers = true;
        break;
      case 'O':
        std::cout << "> Log client output: ON" << std::endl;
        wParams.log_client_output = true;
        break;
      default:
        break;
      }
    }                                             //while

  if(confFile.empty()) {
    if(wParams.debug){
      std::cerr << "> Config file is not specified, creating default worker" << std::endl;
    }
    create_worker(wParams);
    }
  else {
    std::cout << "* Config is set, all other CLI options will be ignored " << std::endl;
    INIReader reader(confFile);
    if (reader.ParseError() < 0) {
      std::cout << "Can't load " << confFile << std::endl;
      exit(1);
      }

    std::vector<std::string> sections;
    sections = reader.GetSections();
    std::vector<std::string>::iterator it;

    for (it = sections.begin(); it != sections.end(); it++) {
      std::string secName = *it;
      std::cerr << "=> " << getpid() << ": Processing config file for " << secName << std::endl;

      if(reader.GetBoolean(secName, "run", false)) {
        read_section_settings(wParams, secName, confFile);
        create_worker(wParams);
        }
      }
    }
  while ((wPID = wait(&status)) > 0) {
    std::cerr << "! Exit status of child with PID " << wPID << ": " << status << std::endl;
    }
  mysql_library_end();

  return EXIT_SUCCESS;
  }
