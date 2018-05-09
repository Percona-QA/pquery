#include <cDbWorker.hpp>

#ifndef PQPGSQLWORKER_HPP
#define PQPGSQLWORKER_HPP

class PgsqlWorker: public DbWorker
  {
  public:
    PgsqlWorker();
    ~PgsqlWorker();
    bool executeTests(struct workerParams&);
    std::shared_ptr<Database> createDbInstance();
    void endDbThread();
  private:
    bool testConnection();
    void workerThread(int number);
  };
#endif
