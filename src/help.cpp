#include "pquery.hpp"
#include <iostream>

void show_help(void) {
  std::cout << " - PQuery version " << PQVERSION << std::endl;
  std::cout << " - Usage: pquery --config-file=pquery.cfg" << std::endl;
  std::cout << " - CLI params has been replaced by config file (INI format)" << std::endl;
  std::cout << " - You can redefine any global param=value pair in host-specific section" << std::endl;
  std::cout << "\nConfig example:\n" << std::endl;
  std::cout <<
    "# contains global params for all workers\n" <<
    "[GLOBAL]\n" <<
    "# The defaults are below\n" <<
    "# The database to connect to\n" <<
    "database = test\n" <<
    "# IP address to connect to, default is AF_UNIX\n" <<
    "address = <empty>\n"    <<
    "# The port to connect to\n"          <<
    "port = 3306\n"           <<
    "# The SQL input file\n" <<
    "infile = pquery.sql\n"     <<
    "# Directory to store logs\n" <<
    "logdir = /tmp\n"           <<
    "# Socket file to use\n" <<
    "socket = /tmp/my.sock\n"   <<
    "# The MySQL userID to be used\n" <<
    "user = test\n"     <<
    "# The MySQL user's password\n" <<
    "password = test\n"        <<
    "# The number of threads to use by worker\n" <<
    "threads = 10\n"             <<
    "# The number of queries per thread\n"
    "queries-per-thread = 10000\n"          <<
    "# Duplicates the log to console when threads=1 and workers=1\n"
    "verbose = No\n"             <<
    "# Log all queries\n" <<
    "log-all-queries = No\n"             <<
    "# Log failed queries\n" <<
    "log-failed-queries = No\n"             <<
    "# Execute SQL randomly\n" <<
    "shuffle = Yes\n"       <<
    "# Extended output of query result\n"
    "log-query-statistics = No\n"             <<
    "# Log query duration in milliseconds\n" <<
    "log-query-duration = No\n\n"             <<
    "[node1.domain.tld]\n" <<
    "address = 10.10.6.10\n" <<
    "# default is No, need to set it explicitly\n" <<
    "run = Yes\n\n" <<
    "[node2.domain.tld]\n" <<
    "address = 10.10.6.11\n" << std::endl;
}
