#include <cstdlib>
#include <iostream>
#include <cassert>
#include <getopt.h>
#include <chrono>
#include <ctime>
#include <cstring>
#include <cmath>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <common.hpp>
#include <cPQuery.hpp>
//
#ifdef HAVE_MYSQL
#include <cMysqlWorker.hpp>
# include <mysql.h>
#endif

#ifdef HAVE_PGSQL
#include <cPgsqlWorker.hpp>
# include <pg_config.h>
#endif

#ifdef HAVE_MONGO
#include <cMongoWorker.hpp>
#endif

PQuery::PQuery() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  configFilePath = "pquery.cfg";
  configReader = NULL;
  dbWorker = NULL;
  }


PQuery::~PQuery() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if (configReader != NULL) { delete configReader; configReader = NULL;}
  if (dbWorker != NULL) { delete dbWorker; dbWorker = NULL; }
  }


#ifdef HAVE_MYSQL
std::string
PQuery::getMySqlClientInfo() {
  return mysql_get_client_info();
  }
#endif

#ifdef HAVE_PGSQL
std::string
PQuery::getPgSqlClientInfo() {
  return std::string(PG_VERSION);
  }
#endif

bool
PQuery::initLogger() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  pqLogger = std::make_shared<Logger>();
  if(pqLogger == NULL) {
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
#ifdef HAVE_MYSQL
  pqLogger->addRecordToLog("* PQuery MySQL client library: " + std::string(MYSQL_FORK) + " v." + getMySqlClientInfo());
#endif
#ifdef HAVE_PGSQL
  pqLogger->addRecordToLog("* PQuery PgSQL client library: PgSQL v." + getPgSqlClientInfo());
#endif
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
PQuery::doCleanup(std::string name) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::string logfile = configReader->Get("master", "logdir", "/tmp") + "/" + name + "_worker.log";
  pqLogger->initLogFile(logfile);
  if (configReader != 0){ delete configReader; configReader = 0; }
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

  wParams.dbtype    = configReader->getDbType(secName, "dbtype", eNONE);

  wParams.port = configReader->GetInteger(secName, "port", 3306);
  wParams.threads = configReader->GetInteger(secName, "threads", 10);
  wParams.queries_per_thread = configReader->GetInteger(secName, "queries-per-thread", 10000);
#ifdef HAVE_MYSQL
#ifdef MAXPACKET
  wParams.maxpacket = configReader->GetInteger(secName, "max-packet-size", MAX_PACKET_DEFAULT);
#endif
#endif
  wParams.verbose = configReader->GetBoolean(secName, "verbose", false);
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


void
PQuery::logWorkerDetails(struct workerParams& Params) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  pqLogger->addRecordToLog("#########################################################################");
  pqLogger->addRecordToLog("## Config name: " + Params.myName);
  pqLogger->addRecordToLog("## DB type: " + dbtype_str(Params.dbtype));
  pqLogger->addRecordToLog("## DB name: " + Params.database);
  pqLogger->addRecordToLog("## DB address: " + Params.address);
  pqLogger->addRecordToLog("## DB username: " + Params.username);
  pqLogger->addRecordToLog("## DB password: " + Params.password);
  pqLogger->addRecordToLog("## DB socket: " + Params.socket);
  pqLogger->addRecordToLog("## DB port: " + std::to_string(Params.port));
#ifdef HAVE_MYSQL
#ifdef MAXPACKET
  pqLogger->addRecordToLog("## MySQL max packet size: " + std::to_string(Params.maxpacket));
#endif
#endif
  pqLogger->addRecordToLog("## PQuery threads: " + std::to_string(Params.threads));
  pqLogger->addRecordToLog("## PQuery queries per thread: " + std::to_string(Params.queries_per_thread));
  pqLogger->addRecordToLog("## PQuery verbosity: " + std::to_string(Params.verbose));
  pqLogger->addRecordToLog("## PQuery shuffle: " + std::to_string(Params.shuffle));
  pqLogger->addRecordToLog("## PQuery infile: " + Params.infile);
  pqLogger->addRecordToLog("## PQuery log directory: " + Params.logdir);
  pqLogger->addRecordToLog("## PQuery log all queries: " + std::to_string(Params.log_all_queries));
  pqLogger->addRecordToLog("## PQuery log failed queries: " + std::to_string(Params.log_failed_queries));
  pqLogger->addRecordToLog("## PQuery log query statistics: " + std::to_string(Params.log_query_statistics));
  pqLogger->addRecordToLog("## PQuery log query duration: " + std::to_string(Params.log_query_duration));
  pqLogger->addRecordToLog("## PQuery log client output: " + std::to_string(Params.log_client_output));
  pqLogger->addRecordToLog("## PQuery log query numbers: " + std::to_string(Params.log_query_numbers));
  pqLogger->addRecordToLog("#########################################################################");
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
    pqLogger->addRecordToLog("=> Cannot fork() child process: " + std::string(std::strerror(errno)));
    return wERROR;
    }

  if(childPID > 0) {
    pqLogger->addRecordToLog("-> Waiting for created worker " + std::to_string(childPID));
    return wMASTER;
    }

  if (childPID == 0) {
    doCleanup(Params.myName);

    if(Params.verbose) {
      logWorkerDetails(Params);
      }

    switch (Params.dbtype) {

#ifdef HAVE_MYSQL
      case eMYSQL:
        dbWorker = new MysqlWorker();
        break;
#endif
#ifdef HAVE_PGSQL
      case ePGSQL:
        dbWorker = new PgsqlWorker();
        break;
#endif
#ifdef HAVE_MONGO
      case eMONGO:
        dbWorker = new MongoWorker();
        break;
#endif
      default:
        std::cerr << "=> Unable to create worker of unsupported type " << dbtype_str(Params.dbtype) << std::endl;
        pqLogger->addRecordToLog("=> PQuery is not compiled with " + dbtype_str(Params.dbtype));
        return wERROR;
      }

    if(dbWorker == NULL) {
      pqLogger->addRecordToLog("=> Error creating worker of type  " + dbtype_str(Params.dbtype));
      pqLogger->addRecordToLog("=> Something went really wrong, exiting...");
      return wERROR;
      }

//TODO
    int retcode;
    dbWorker->setupLogger(pqLogger);
    retcode = dbWorker->executeTests(Params);

    if(retcode != 0) {
      return wERROR;
      }

    return wCHILD;                                //fake
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
    pqLogger->addRecordToLog("=> Worker returned error for " + secName);
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
      wRETCODE wrc = createWorkerWithParams(secName);
      switch(wrc) {
        case wERROR:
          return false;
        case wCHILD:
          return true;
        default:
          break;
        }
      }
    }                                             // for()

  pid_t wPID;
  int status;
  bool retvalue = true;                           //uninitialised is always false

  while ((wPID = wait(&status)) > 0) {
    if(status != 0) { retvalue = false; }
    pqLogger->addRecordToLog("=> Exit status of child with PID " + std::to_string(wPID) + ": " + std::to_string(status));
    }
  return retvalue;
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
#ifdef HAVE_MYSQL
  std::cout << "* PQuery MySQL client library: " + std::string(MYSQL_FORK) + " v." + getMySqlClientInfo() << std::endl;
#endif
#ifdef HAVE_PGSQL
  std::cout <<  "* PQuery PgSQL client library: PgSQL v." + getPgSqlClientInfo() << std::endl;
#endif
#ifdef HAVE_MONGO
  std::cout <<  "* PQuery MongoDB client library: MongoDB v." + getMongoDBClientInfo() << std::endl;
#endif
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
