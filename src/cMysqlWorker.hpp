#include <cDbWorker.hpp>
#include <mysql.h>

#ifndef PQMYSQLWORKER_HPP
#define PQMYSQLWORKER_HPP

class MysqlWorker: public DbWorker
  {
  public:
    MysqlWorker();
    ~MysqlWorker();
    bool executeTests(struct workerParams&);

  private:
    std::string getServerVersion(MYSQL*);
    bool testConnection();
  };
#endif
