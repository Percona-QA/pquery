
#ifdef DEBUG
#include <iostream>
#endif
#include <cPgsqlWorker.hpp>
#include <cPgsqlDatabase.hpp>

PgsqlWorker::PgsqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  }


PgsqlWorker::~PgsqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  }


bool
PgsqlWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  std::shared_ptr<Database> pgsqlDB = createDbInstance();

  if(!pgsqlDB->connect(mParams)) {
    wLogger->addRecordToLog("=> Unable to connect! PgSQL error " + pgsqlDB->getErrorString());
    return false;
    }

  wLogger->addRecordToLog("-> Successfully connected to " + pgsqlDB->getHostInfo());
  wLogger->addRecordToLog("-> Server version: " + pgsqlDB->getServerVersion());
  return true;
  }


std::shared_ptr<Database>
PgsqlWorker::createDbInstance() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::shared_ptr<Database> pgsqlDB = std::make_shared<PgsqlDatabase>();
  return pgsqlDB;
  }


void
PgsqlWorker::endDbThread() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  }
