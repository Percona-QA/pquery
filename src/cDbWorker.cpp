#ifdef DEBUG
#include <iostream>
#endif
#include <cstring>
#include <fstream>
#include <sstream>
#include <cDbWorker.hpp>

DbWorker::DbWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  workers.clear();
  failed_queries_total = 0;
  performed_queries_total = 0;
  }


DbWorker::~DbWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  }


void
DbWorker::adjustRuntimeParams() {
/* log replaying */
  if(!mParams.shuffle) {
    wLogger->addRecordToLog("=> Setting # of threads to 1 for log replaying due to \"shuffle = False\"");
    mParams.threads = 1;
//
    wLogger->addRecordToLog("=> Setting queries per thread to " + std::to_string(queryList->size()) + " due to \"shuffle = False\" (size of infile)");
    mParams.queries_per_thread = queryList->size();
    }
/* END log replaying */
  }


void
DbWorker::writeFinalReport() {
  std::ostringstream exitmsg;
  exitmsg.precision(2);
  exitmsg << std::fixed;
  exitmsg << "-> WORKER SUMMARY: " << failed_queries_total << "/" << performed_queries_total << " queries failed, (" <<
    (performed_queries_total - failed_queries_total)*100.0/performed_queries_total << "% were successful)";
  wLogger->addRecordToLog(exitmsg.str());
  }


void
DbWorker::storeParams(struct workerParams& wParams) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mParams = wParams;
  }


void
DbWorker::setupLogger(std::shared_ptr<Logger> logger) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  wLogger = logger;
  }

bool
DbWorker::isComment(std::string& line){
  size_t first = line.find_first_not_of(' ');
  if (std::string::npos == first){ return false; }
  size_t last = line.find_last_not_of(' ');
  auto qStr = line.substr(first, (last - first + 1));
  return (
  (qStr.rfind("#", 0) == 0) ||
  (qStr.rfind(";", 0) == 0) ||
  (qStr.rfind("//", 0) == 0)
  );
}

bool
DbWorker::loadQueryList() {
  queryList = std::make_shared<std::vector<std::string>>();
  if(queryList == NULL) {
    wLogger->addRecordToLog("=> Unable to create Query List, exiting...");
    return false;
    }
  std::ifstream sqlfile_in;
  sqlfile_in.open(mParams.infile);
  if (!sqlfile_in.is_open()) {
    wLogger->addRecordToLog("=> Unable to open infile " + mParams.infile + ": " + strerror(errno));
    return false;
    }
  std::string line;
  while (getline(sqlfile_in, line)) {

    if((!line.empty()) && (!isComment(line))){
      queryList->push_back(line);
      }
    }
  wLogger->addRecordToLog("-> Loaded " + std::to_string(queryList->size()) + " lines from infile: " + mParams.infile);
  if (sqlfile_in.is_open()) { sqlfile_in.close(); }
  return true;
  }

void
DbWorker::spawnWorkerThreads(){
  workers.resize(mParams.threads);
    for (int i=0; i<mParams.threads; i++) {
    workers[i] = std::thread(&DbWorker::workerThread, this, i);
    }
      for (int i=0; i<mParams.threads; i++) {
    workers[i].join();
    }
}

bool
DbWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  return true;
  }


bool
DbWorker::executeTests(struct workerParams wParams) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  storeParams(wParams);

  if(!testConnection()) { return false; }
  if(!loadQueryList()) { return false; }
  adjustRuntimeParams();
  spawnWorkerThreads();
  writeFinalReport();
  return true;
  }
