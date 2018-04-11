#include <cstdlib>
#include <iostream>
#include <cassert>
#include <getopt.h>
#include <chrono>
#include <ctime>

#include <common.hpp>
#include <cPQuery.hpp>

PQuery::PQuery() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  configFilePath = "pquery.cfg";
  configReader = 0;
  pqLogger = 0;
  }


PQuery::~PQuery() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if (configReader != 0) { delete configReader; configReader = 0;}
  if (pqLogger != 0) { delete pqLogger; pqLogger = 0; }
  }


bool
PQuery::initLogger() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(pqLogger == 0) {
    pqLogger = new Logger();
    }
  if(!pqLogger) {
    std::cerr << "Unable to init logging subsystem" << std::endl;
    return false;
    }

  assert(configReader != 0);

  std::string masterLogFile;
  std::string master_logdir = configReader->Get("master", "logdir", "/tmp");
  std::string master_logfile = configReader->Get("master", "logfile", "pquery3-master.log");
  masterLogFile = master_logdir + FSSEP + master_logfile;
  if(!pqLogger->initLogFile(masterLogFile)){ return false; }
  return true;
  }


bool
PQuery::logVersionInfo() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  auto now = std::chrono::system_clock::now();
  std::time_t start_time = std::chrono::system_clock::to_time_t(now);
  pqLogger->addRecordToLog("* PQuery version: " + std::string(PQVERSION));
  pqLogger->addRecordToLog("* PQuery revision: " + std::string(PQREVISION));
  pqLogger->addRecordToLog("* PQuery revision date: " + std::string(PQRELDATE));
  pqLogger->addRecordToLog("* Pquery master with PID " + std::to_string(getpid()) + " started at " + std::string(std::ctime(&start_time)));
  return true;
  }


bool
PQuery::initConfig() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  configReader = new INIReader(configFilePath);
  int parseerr = configReader->ParseError();

  if (parseerr < 0) {
    std::cerr << "Can't load config from file \"" + configFilePath + "\"" << std::endl;
    return false;
    }
  if (parseerr > 0) {
    std::cerr << "Config parse error!" << std::endl;
    std::cerr << "File: " << configFilePath << std::endl;
    std::cerr << "Line: " << parseerr << std::endl;
    return false;
    }
  return true;
  }


bool
PQuery::prepareToRun() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(!initConfig()) { return false; }
  if(!initLogger()) { return false; }
  if(!logVersionInfo()) { return false; }

  return true;
  }


void
PQuery::setupWorkerParams(struct workerParams& wParams, std::string secName) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  wParams.myName    = secName;
  wParams.address   = configReader->Get(secName, "address", "localhost");
  wParams.username  = configReader->Get(secName, "user", "test");
  wParams.password  = configReader->Get(secName, "password", "");
  wParams.socket    = configReader->Get(secName, "socket", "/tmp/my.sock");
  wParams.database  = configReader->Get(secName, "database", "");
  wParams.dbtype    = configReader->Get(secName, "dbtype", "");

  wParams.port = configReader->GetInteger(secName, "port", 3306);
  wParams.threads = configReader->GetInteger(secName, "threads", 10);
  wParams.queries_per_thread = configReader->GetInteger(secName, "queries-per-thread", 10000);
#ifdef MAXPACKET
  wParams.maxpacket = configReader->GetInteger(secName, "max-packet-size", MAX_PACKET_DEFAULT);
#endif
  wParams.verbose = configReader->GetBoolean(secName, "verbose", false);
  wParams.debug = configReader->GetBoolean(secName, "debug", false);
  wParams.shuffle = configReader->GetBoolean(secName, "shuffle", true);
  wParams.infile = configReader->Get(secName, "infile", "pquery.sql");
  wParams.logdir = configReader->Get(secName, "logdir", "/tmp");
//
  wParams.log_all_queries = configReader->GetBoolean(secName, "log-all-queries", false);
  wParams.log_succeeded_queries = configReader->GetBoolean(secName, "log-succeded-queries", false);
  wParams.log_failed_queries = configReader->GetBoolean(secName, "log-failed-queries", false);
  wParams.log_query_statistics = configReader->GetBoolean(secName, "log-query-statistics",  false);
  wParams.log_query_duration = configReader->GetBoolean(secName, "log-query-duration", false);
  wParams.log_client_output = configReader->GetBoolean(secName, "log-client-output", false);
  wParams.log_query_numbers = configReader->GetBoolean(secName, "log-query-numbers", false);
  }


wRETCODE
PQuery::createWorkerProcess(struct workerParams& Params) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  pqLogger->flushLog();
  pid_t childPID;
  childPID = fork();
  if(childPID < 0) {
    pqLogger->addRecordToLog("=> Cannot fork() child process: " + std::string(strerror(errno)));
    return wERROR;
    }
  if(childPID > 0) {
    pqLogger->addRecordToLog("-> Waiting for created worker " + std::to_string(childPID));
    return wMASTER;
    }
  if (childPID == 0) {
    Worker newWorker(Params);
    sleep(10);
//    newWorker.startWork(Params);

//    newNode.setAllParams(Params);
//    exitStatus = newNode.startWork();
//}
//    return exitStatus;
    return wCHILD;                            //fake
    }
  return wDEFAULT;
  }


wRETCODE
PQuery::createWorkerWithParams(std::string secName) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  struct workerParams wParams;
  setupWorkerParams(wParams, secName);
  wRETCODE wrc = createWorkerProcess(wParams);
  if( wrc == wERROR) {
    pqLogger->addRecordToLog("=> Error creating worker for " + secName);
    }
  return wrc;
  }


bool
PQuery::runWorkers() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::vector<std::string> sections;
  sections = configReader->GetSections();
  std::vector<std::string>::iterator it;

  for (it = sections.begin(); it != sections.end(); it++) {
    std::string secName = *it;
    if (toLowerCase(secName) == "master") { continue; }
    pqLogger->addRecordToLog("-> Checking " + secName + " params...");
    if(configReader->GetBoolean(secName, "run", false)) {
      pqLogger->addRecordToLog("-> Running worker for " + secName);
      if (!createWorkerWithParams(secName)) {
        return false;
        }
      }
    }
  pid_t wPID;
  int status;
  while ((wPID = wait(&status)) > 0) {
    pqLogger->addRecordToLog("=> Exit status of child with PID " + std::to_string(wPID) + ": " + std::to_string(status));
    }
  return true;
  }


int
PQuery::run() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  showVersion();
  if(!prepareToRun()){ return EXIT_FAILURE; }
  if(!runWorkers()){ return EXIT_FAILURE; }
  return EXIT_SUCCESS;
  }


void
PQuery::showVersion() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::cout << "* PQuery version: "       << PQVERSION << std::endl;
  std::cout << "* PQuery revision: "      << PQREVISION << std::endl;
  std::cout << "* PQuery release date: "  << PQRELDATE << std::endl;
  }


void
PQuery::showHelp() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::cout << " - Usage: pquery --config-file=pquery.cfg" << std::endl;
  std::cout << " - CLI params has been replaced by config file (INI format)" << std::endl;
  std::cout << " - You can redefine any global param=value pair in host-specific section" << std::endl;
  std::cout << "\nConfig example:\n" << std::endl;
  std::cout <<
    "# Section for master process\n" <<
    "[master]\n" <<
    "# Directory to store logs\n" <<
    "logdir = /tmp\n" <<
    "# Logfile for master process\n" <<
    "logfile = pquery3-master.log\n\n" <<
    "[node0.domain.tld]\n" <<
    "# The database to connect to\n" <<
    "database = \n" <<
    "# Database type, may be MySQL OR PostgreSQL\n" <<
    "dbtype = <empty>\n" <<
    "# IP address to connect to, default is AF_UNIX\n" <<
    "address = <empty>\n"    <<
    "# The port to connect to\n"          <<
    "port = 3306\n"           <<
    "# The SQL input file\n" <<
    "infile = pquery.sql\n"     <<
    "# Directory to store logs\n" <<
    "logdir = /tmp\n"           <<
    "# Socket file to use\n" <<
    "socket = /tmp/my.sock\n"   <<
    "# The DB userID to be used\n" <<
    "user = test\n"     <<
    "# The DB user's password\n" <<
    "password = test\n"        <<
    "# The number of threads to use by worker\n" <<
    "threads = 1\n"             <<
    "# The number of queries per thread\n"
    "queries-per-thread = 10000\n"          <<
    "# Duplicates the log to console when threads=1 and workers=1\n"
    "verbose = No\n"             <<
    "# Log all queries\n" <<
    "log-all-queries = No\n"             <<
    "# Log succeeded queries\n" <<
    "log-succeeded-queries = No\n"             <<
    "# Log failed queries\n" <<
    "log-failed-queries = No\n"             <<
    "# Execute SQL randomly\n" <<
    "shuffle = Yes\n"       <<
    "# Extended output of query result\n"
    "log-query-statistics = No\n"             <<
    "# Log query duration in milliseconds\n" <<
    "log-query-duration = No\n"             <<
    "# Log output from executed query (separate log)\n" <<
    "log-client-output = No\n"             <<
    "# Log query numbers along the query results and statistics\n" <<
    "log-query-number = No\n\n" <<
    "[node1.domain.tld]\n" <<
    "address = 10.10.6.10\n" <<
    "# default for \"run\" is No, need to set it explicitly\n" <<
    "run = Yes\n\n" <<
    "[node1.domain.tld]\n" <<
    "address = 10.10.6.11\n" << std::endl;
  }


bool
PQuery::parseCliOptions(int argc, char* argv[]) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  int c;
  while(true) {
    static struct option long_options[] = {
// config file with all options
      {"config-file", required_argument, 0, 'c'},
      {"master-logfile", required_argument, 0, 'L'},
      {"help", no_argument, 0, 'h'},
      {"version", no_argument, 0, 'v'},
// finally
      {0, 0, 0, 0}
      };
    int option_index = 0;
    c = getopt_long_only(argc, argv, "c:L:hv", long_options, &option_index);
    if (c == -1) {
      break;
      }
    switch (c) {
      case 'c':
        setConfigFilePath(optarg);
        break;
      case 'L':
        setLogFilePath(optarg);
        break;
      case 'h':
        showHelp();
        return false;
      case 'v':
        showVersion();
        return false;
      default:
        break;
      }

    }
  return true;
  }
