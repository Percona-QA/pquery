#include "pquery.hpp"
#include <iostream>

void show_help(void) {
  std::cout << " - PQuery version " << PQVERSION << std::endl;
  std::cout << " - General usage: pquery --user=USER --password=PASSWORD --database=DATABASE" << std::endl;
  std::cout <<
    "----------------------------------------------------------------------------\n"  <<
    "| OPTION               | EXPLANATION                         | DEFAULT      |\n" <<
    "----------------------------------------------------------------------------\n"  <<
    "--database             | The database to connect to          | test\n"           <<
    "--address              | IP address to connect to            | -\n"              <<
    "--port                 | The port to connect to              | 3306\n"           <<
    "--infile               | The SQL input file                  | pquery.sql\n"     <<
    "--logdir               | Log directory                       | /tmp\n"           <<
    "--socket               | Socket file to use                  | /tmp/my.sock\n"   <<
    "--user                 | The MySQL userID to be used         | shell user\n"     <<
    "--password             | The MySQL user's password           | <empty>\n"        <<
    "--threads              | The number of threads to use        | 10\n"             <<
    "--queries-per-thread   | The number of queries per thread    | 10000\n"          <<
    "--verbose              | Produce verbose output              | no\n"             <<
    "--log-all-queries      | Log all queries                     | no\n"             <<
    "--log-failed-queries   | Log failed queries                  | no\n"             <<
    "--no-shuffle           | Execute SQL sequentially            | randomly\n"       <<
    "--log-query-statistics | Extended output of query result     | no\n"             <<
    "--log-query-duration   | Log query duration in milliseconds  | no\n"             <<
    "--test-connection      | Test connection to server and exit  | no\n"             <<
    "----------------------------------------------------------------------------"    << std::endl;
}
