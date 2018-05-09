#include <cDatabase.hpp>
#include <libpq-fe.h>

#ifndef _PGSQLDATABASE_
#define _PGSQLDATABASE_

class PgsqlDatabase : public Database
  {
  public:
    PgsqlDatabase();
    ~PgsqlDatabase();
    std::string getServerVersion();
    std::string getHostInfo();
    std::string getErrorString();
    bool connect(struct workerParams&);
    inline std::uint64_t getAffectedRows();
    bool performRealQuery(std::string);
    void processQueryOutput();
    std::uint32_t getWarningsCount();

  private:
    PGconn* conn;
  };
#endif
