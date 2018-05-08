#ifdef DEBUG
#include <iostream>
#endif
#include <cDatabase.hpp>

Database::~Database() {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
  }


std::chrono::duration<double>
Database::getQueryDuration(){
    return std::chrono::duration<double> ((end - begin).count() * 1000);
}
