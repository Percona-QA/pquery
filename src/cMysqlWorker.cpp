#include <iostream>
#include <hCommon.hpp>
#include <cMysqlWorker.hpp>
#include <cMysqlDatabase.hpp>

MysqlWorker::MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if (mysql_library_init(0, NULL, NULL)) {
    throw std::runtime_error("=> Could not initialize MySQL client library");
    }
  }


MysqlWorker::~MysqlWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mysql_library_end();
  }


bool
MysqlWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  std::shared_ptr<Database> mysqlDB = createDbInstance();

  if(!mysqlDB->connect(mParams)) {
    wLogger->addRecordToLog("=> Unable to connect! MySQL error " + mysqlDB->getErrorString());
    return false;
    }

  wLogger->addRecordToLog("-> Successfully connected to " + mysqlDB->getHostInfo());
  wLogger->addRecordToLog("-> Server version: " + mysqlDB->getServerVersion());
  return true;

  }


std::shared_ptr<Database>
MysqlWorker::createDbInstance() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  std::shared_ptr<Database> mysqlDB = std::make_shared<MysqlDatabase>();
  return mysqlDB;
  }


void
MysqlWorker::endDbThread() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  mysql_thread_end();
  }
