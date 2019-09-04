/*
 =========================================================
 #       Created by Alexey Bychko, Percona LLC           #
 #     Expanded by Roel Van de Paar, Percona LLC         #
 =========================================================
*/

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

#include "common.hpp"
#include "node.hpp"
#include "pquery.hpp"
#include "random_test.hpp"
#include <INIReader.hpp>
#include <mysql.h>

std::string confFile;
pid_t childPID, wPID;
int status;
void set_defaults(struct workerParams &Params) {
  // initialize all fields with default values
  Params.database = opt_string(DATABASE);
  Params.address = opt_string(ADDRESS);
  Params.socket = opt_string(SOCKET);
  Params.username = opt_string(USER);
  Params.password = opt_string(PASSWORD);
  Params.logdir = opt_string(LOGDIR);
  Params.infile = opt_string(INFILE);
  Params.port = opt_int(PORT);
  Params.threads = opt_int(THREADS);
  Params.queries_per_thread = opt_int(QUERIES_PER_THREAD);
  Params.log_all_queries = opt_bool(LOG_ALL_QUERIES);
  Params.log_succeeded_queries = opt_bool(LOG_SUCCEDED_QUERIES);
  Params.log_failed_queries = opt_bool(LOG_FAILED_QUERIES);
  Params.log_query_statistics = opt_bool(LOG_QUERY_STATISTICS);
  Params.log_query_duration = opt_bool(LOG_QUERY_DURATION);
  Params.log_client_output = opt_bool(LOG_CLIENT_OUTPUT);
  Params.log_query_numbers = opt_bool(LOG_QUERY_NUMBERS);
  Params.shuffle = opt_bool(NO_SHUFFLE);
  Params.test_connection = opt_bool(TEST_CONNECTION);
}

void read_section_settings(struct workerParams &wParams, std::string secName,
                           std::string confFile) {
  set_defaults(wParams);
  INIReader reader(confFile);
  wParams.myName = secName;
  wParams.address = reader.Get(secName, "address", "localhost");
  wParams.username = reader.Get(secName, "user", "test");
  wParams.password = reader.Get(secName, "password", "");
  wParams.socket = reader.Get(secName, "socket", "/tmp/my.sock");
  wParams.database = reader.Get(secName, "database", "");
  wParams.port = reader.GetInteger(secName, "port", 3306);
  wParams.threads = reader.GetInteger(secName, "threads", 10);
  wParams.queries_per_thread =
      reader.GetInteger(secName, "queries-per-thread", 10000);
#ifdef MAXPACKET
  wParams.maxpacket =
      reader.GetInteger(secName, "max-packet-size", MAX_PACKET_DEFAULT);
#endif
  wParams.verbose = reader.GetBoolean(secName, "verbose", false);
  wParams.debug = reader.GetBoolean(secName, "debug", false);
  wParams.shuffle = reader.GetBoolean(secName, "shuffle", true);
  wParams.infile = reader.Get(secName, "infile", "pquery.sql");
  wParams.logdir = reader.Get(secName, "logdir", "/tmp");
  wParams.test_connection =
      reader.GetBoolean(secName, "test-connection", false);
  wParams.log_all_queries =
      reader.GetBoolean(secName, "log-all-queries", false);
  wParams.log_succeeded_queries =
      reader.GetBoolean(secName, "log-succeded-queries", false);
  wParams.log_failed_queries =
      reader.GetBoolean(secName, "log-failed-queries", false);
  wParams.log_query_statistics =
      reader.GetBoolean(secName, "log-query-statistics", false);
  wParams.log_query_duration =
      reader.GetBoolean(secName, "log-query-duration", false);
  wParams.log_client_output =
      reader.GetBoolean(secName, "log-client-output", false);
  wParams.log_query_numbers =
      reader.GetBoolean(secName, "log-query-numbers", false);
}

void create_worker(struct workerParams &Params) {
  childPID = fork();
  if (childPID < 0) {
    std::cerr << "=> Cannot fork() child process: " << strerror(errno)
              << std::endl;
    exit(EXIT_FAILURE);
  }
  if (childPID > 0) {
    std::cerr << "* Waiting for created worker " << childPID << std::endl;
  }
  if (childPID == 0) {
    int exitStatus;
    {
      Node newNode;
      newNode.setAllParams(Params);
      exitStatus = newNode.startWork();
    }
    exit(exitStatus);
  }
}

int main(int argc, char *argv[]) {

  std::ios_base::sync_with_stdio(false);
  confFile.clear();
  static struct workerParams wParams;
  add_options();
  int c;
  while (true) {
    struct option long_options[Option::MAX];
    /* static struct option long_options[] =
          {"config-help", no_argument, 0, 'I'},
          {"cli-help", no_argument, 0, 'C'},
          {"verbose", no_argument, 0, 'v'},
          {"debug", no_argument, 0, 'E'},
    */
    int option_index = 0;

    int i = 0;
    for (auto op : *options) {
      if (op == nullptr)
        continue;
      long_options[i++] = {op->getName(), op->getArgs(), 0, op->getOption()};
    };
    long_options[i] = {0, 0, 0, 0};

    c = getopt_long_only(argc, argv, "c:d:a:i:l:s:p:u:P:t:q:vAEFNLDTNOS",
                         long_options, &option_index);

    if (c == -1) {
      break;
      exit(EXIT_FAILURE);
    }

    switch (c) {
    case 'h':
      if (optarg) {
        std::string s(optarg);
        show_help(s);
      } else {
        show_help();
      }
      exit(EXIT_FAILURE);
    case 'I':
      show_config_help();
      exit(EXIT_FAILURE);
    case 'i':
      wParams.infile = optarg;
      break;
    case 'C':
      show_cli_help();
      exit(EXIT_FAILURE);
    case 'v':
      std::cout << "> Verbose mode: ON" << std::endl;
      show_help("verbose");
      wParams.verbose = true;
      break;
    case 'E':
      std::cout << "> Debug mode: ON" << std::endl;
      wParams.debug = true;
      break;
    case Option::MYSQLD_SERVER_OPTION:
      add_server_options(optarg);
      break;
    case Option::SERVER_OPTION_FILE:
      add_server_options_file(optarg);
      break;
    default:
      if (c >= Option::MAX) {
        break;
      }
      if (options->at(c) == nullptr) {
        throw std::runtime_error("INVALID OPTION");
        break;
      }
      auto op = options->at(c);

      /* set command line */
      op->set_cl();

      if (op->getArgs() == required_argument) {
        switch (op->getType()) {
        case Option::INT:
          op->setInt(optarg);
          break;
        case Option::STRING:
          op->setString(optarg);
          break;
        case Option::BOOL:
          std::string s(optarg);
          op->setBool(s);
          break;
        }
      } else if (op->getArgs() == no_argument) {
        op->setBool(true);
      } else if (op->getArgs() == optional_argument) {
        if (optarg) {
          std::string s(optarg);
          op->setBool(s);
        } else
          op->setBool(true);
        break;
      }
    }
  } // while

  set_defaults(wParams); // reset all settings in struct to defaults

  if (options->at(Option::CONFIGFILE)->getString().size() > 0)
    confFile = options->at(Option::CONFIGFILE)->getString();

  if (confFile.empty()) {
    /*single node and command line */

    create_worker(wParams);
  } else {
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
      std::cerr << "=> Master process with PID " << getpid()
                << ": Processing config file for " << secName << std::endl;

      if (reader.GetBoolean(secName, "run", false)) {
        read_section_settings(wParams, secName, confFile);
        create_worker(wParams);
      }
    }
  }
  while ((wPID = wait(&status)) > 0) {
    std::cerr << "! Exit status of child with PID " << wPID << ": " << status
              << std::endl;
  }

  mysql_library_end();
  delete_options();

  return EXIT_SUCCESS;
}
