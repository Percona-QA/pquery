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

  private:
    inline unsigned long long getAffectedRows(MYSQL * connection);
    bool testConnection();
    void workerThread(int number);
  };
#endif
