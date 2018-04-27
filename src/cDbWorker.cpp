#ifdef DEBUG
#include <iostream>
#endif
#include <cDbWorker.hpp>

DbWorker::DbWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  }


DbWorker::~DbWorker() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  }


void
DbWorker::storeParams(struct workerParams wParams) {
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
DbWorker::testConnection() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  return true;
  }


int
DbWorker::executeTests(struct workerParams wParams) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  storeParams(wParams);

  if(!testConnection()){ return EXIT_FAILURE; }

  return EXIT_SUCCESS;
  }
