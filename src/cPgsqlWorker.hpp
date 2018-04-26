#include <cDbWorker.hpp>

#ifndef PQPGSQLWORKER_HPP
#define PQPGSQLWORKER_HPP

class PgsqlWorker: public DbWorker
  {
  public:
    PgsqlWorker();
    int executeTests(struct workerParams&);
  };
#endif
