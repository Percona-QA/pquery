#include <cPgsqlWorker.hpp>
#include <iostream>

PgsqlWorker::PgsqlWorker() {

  }


PgsqlWorker::~PgsqlWorker() {

  }


bool
PgsqlWorker::executeTests(struct workerParams&) {
  return 0;
  }


bool
PgsqlWorker::testConnection() {
  return true;
  }


void
PgsqlWorker::workerThread(int number) {
  std::cout << "Thread #" << number << std::endl;
  }
