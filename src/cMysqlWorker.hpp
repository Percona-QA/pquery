#include <cDbWorker.hpp>

#ifndef PQMYSQLWORKER_HPP
#define PQMYSQLWORKER_HPP

class MysqlWorker: public DbWorker
  {
  public:
    MysqlWorker();
    int executeTests(struct workerParams&);
  };
#endif
