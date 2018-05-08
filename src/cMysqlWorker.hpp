#include <cDbWorker.hpp>
#include <mysql.h>

#ifndef PQMYSQLWORKER_HPP
#define PQMYSQLWORKER_HPP

class MysqlWorker: public DbWorker
  {
  public:
    MysqlWorker();
    ~MysqlWorker();
    std::shared_ptr<Database> createDbInstance();
    void endDbThread();

  private:
    bool testConnection();
    void workerThread(int number);
  };
#endif
