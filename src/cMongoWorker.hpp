#include <cDbWorker.hpp>

#ifndef PQMONGOWORKER_HPP
#define PQMONGOWORKER_HPP

class MongoWorker: public DbWorker
  {
  public:
    MongoWorker();
    int executeTests(struct workerParams&);
  };
#endif
