#ifdef DEBUG
#include <iostream>
#endif
#include <cDatabase.hpp>

Database::Database() {
  failed_queries = 0;
  performed_queries = 0;
  max_con_fail_count = 0;
  }


Database::~Database() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  }


double
Database::getQueryDurationMs() {
  std::chrono::duration<double, std::milli> duration = end - begin;
  return duration.count();
  }


bool
Database::performQuery(std::string query) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  bool success;
  begin = std::chrono::steady_clock::now();
  success = performRealQuery(query);
  end = std::chrono::steady_clock::now();

  if(success) {
    max_con_fail_count=0;
    }
  else {
    failed_queries++;
    max_con_fail_count++;
    }

  performed_queries++;
  return success;
  }
