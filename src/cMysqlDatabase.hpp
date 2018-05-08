#include <cDatabase.hpp>
#include <mysql.h>

#ifndef _MYSQLDATABASE_
#define _MYSQLDATABASE_

class MysqlDatabase : public Database
  {

  public:
    MysqlDatabase();
    ~MysqlDatabase();
    std::string getServerVersion();
    std::string getHostInfo();
    std::string getErrorString();
    bool connect(struct workerParams&);
    bool init();
    inline uint64_t getAffectedRows();
    bool performRealQuery(std::string);
    std::string getQueryOutput();
    uint32_t getWarningsCount();
  private:
    MYSQL* conn;

  };
#endif
