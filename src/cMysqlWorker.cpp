#include <iostream>
#include <cMysqlWorker.hpp>

MysqlWorker::MysqlWorker() {

  }


int
MysqlWorker::executeTests(struct workerParams& params) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  if(!params.myName.empty()) {
    return 0;
    }
  return 0;
  }
