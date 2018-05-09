
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
