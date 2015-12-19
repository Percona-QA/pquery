#include "pquery.hpp"


void show_help(void) {
  printf(" - PQuery version %s\n", PQVERSION);
  printf(" - General usage: pquery --user=USER --password=PASSWORD --database=DATABASE\n");
  printf(
    "----------------------------------------------------------------------------\n"
    "| OPTION              | EXPLANATION                         | DEFAULT      |\n"
    "----------------------------------------------------------------------------\n"
    "--database           | The database to connect             | test\n"
    "--address            | IP address to connect to            | -\n"
    "--port               | The port to connect to              | 3306\n"
    "--infile             | The SQL input file                  | pquery.sql\n"
    "--logdir             | Log directory                       | /tmp\n"
    "--socket             | Socket file to use                  | /tmp/my.sock\n"
    "--user               | The MySQL userID to be used         | shell user\n"
    "--password           | The MySQL user's password           | <empty>\n"
    "--threads            | The # of threads to use             | 10\n"
    "--queries-per-thread | The number of queries per thread :) | 10000\n"
    "--verbose            | Produce verbose output              | no\n"
    "--log-all-queries    | Log all queries :)                  | no\n"
    "--log-failed-queries | Log failed queries :)               | no\n"
    "--no-shuffle         | Execute SQL sequentially            | randomly\n"
    "--query-analysis     | Extended output of query result     | no\n"
    "--log-query-duration | Query duration in milliseconds      | no\n"
    "--test-connection    | Perform connect to server and exit  | no\n"
    "----------------------------------------------------------------------------\n"
  );
}
