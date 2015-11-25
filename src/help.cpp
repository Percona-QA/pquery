#include "pquery.hpp"

void
show_help(void) {
  printf(" - PQuery version %s\n", PQVERSION);
  printf("General usage: pquery --user=USER --password=PASSWORD --database=DATABASE\n\n");
  printf(
    " OPTION              | EXPLANATION                         | DEFAULT\n"
    "---------------------------------------------------------------------\n"
    "--database           | The database to connect             | test\n"
    "--address            | IP address to connect to            | -\n"
    "--port               | The port to connect to              | 3306\n"
    "--infile             | The SQL input file                  | pquery.sql\n"
    "--logdir             | Log directory                       | /tmp\n"
    "--socket             | Socket file to use                  | /tmp/my.sock\n"
    "--user               | The MySQL userID to be used         | no default\n"
    "--password           | The MySQL user's password           | no default\n"
    "--threads            | The # of threads to use             | 1\n"
    "--queries-per-thread | The number of queries per thread :) | 100000\n"
    "--verbose            | Produce verbose output              | no\n"
    "--log-all-queries    | Log all queries :)                  | no\n"
    "--log-failed-queries | Log failed queries :)               | no\n"
    "--no-shuffle         | Execute SQL sequentially            | randomly\n\n"
  );
}
