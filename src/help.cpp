#include "pquery.hpp"
#include <iostream>
#include "common.hpp"

void
print_version(void) {
  std::cout << " - PQuery v" << PQVERSION << "-" << PQREVISION << " compiled with " << FORK << "-" << mysql_get_client_info() << std::endl;
  }


void
show_help() {
  print_version();
  std::cout << " - pquery supports two different use/configuration modes; commandline or INI-file. Please see specific help:" << std::endl;
  std::cout << "=> pquery --config-help for INI-config help, this mode supports SINGLE and MULTIPLE node(s)" << std::endl;
  std::cout << "=> pquery --cli-help for commandline options, this mode supports a SINGLE node only" << std::endl;
  }


void
show_cli_help(void) {
  print_version();
  std::cout << " - General usage: pquery --user=USER --password=PASSWORD --database=DATABASE" << std::endl;
  std::cout << "=> pquery doesn't support multiple nodes when using commandline options mode!" << std::endl;
  std::cout <<
    "-----------------------------------------------------------------------------------------\n" <<
    "| OPTION               | EXPLANATION                                  | DEFAULT         |\n" <<
    "-----------------------------------------------------------------------------------------\n" <<
    "--database             | The database to connect to                   | test\n"              <<
    "--address              | IP address to connect to                     | -\n"                 <<
    "--port                 | The port to connect to                       | 3306\n"              <<
    "--infile               | The SQL input file                           | pquery.sql\n"        <<
    "--logdir               | Log directory                                | /tmp\n"              <<
    "--socket               | Socket file to use                           | /tmp/my.sock\n"      <<
    "--user                 | The MySQL userID to be used                  | shell user\n"        <<
    "--password             | The MySQL user's password                    | <empty>\n"           <<
    "--threads              | The number of threads to use                 | 1\n"                 <<
    "--queries-per-thread   | The number of queries per thread             | 10000\n"             <<
    "--verbose              | Duplicates the log to console when threads=1 | no\n"                <<
    "--log-all-queries      | Log all queries (succeeded and failed)       | no\n"                <<
    "--log-succeeded-queries| Log succeeded queries                        | no\n"                <<
    "--log-failed-queries   | Log failed queries                           | no\n"                <<
    "--no-shuffle           | Execute SQL sequentially                     | randomly\n"          <<
    "--log-query-statistics | Extended output of query result              | no\n"                <<
    "--log-query-duration   | Log query duration in milliseconds           | no\n"                <<
    "--test-connection      | Test connection to server and exit           | no\n"                <<
    "--log-query-number     | Write query # to logs                        | no\n"                <<
    "--log-client-output    | Log query output to separate file            | no\n"                <<
    "-----------------------------------------------------------------------------------------"   << std::endl;
  }


void
show_config_help(void) {

  print_version();

  std::cout << " - Usage: pquery --config-file=pquery.cfg" << std::endl;
  std::cout << " - CLI params has been replaced by config file (INI format)" << std::endl;
  std::cout << " - You can redefine any global param=value pair in host-specific section" << std::endl;
  std::cout << "\nConfig example:\n" << std::endl;
  std::cout <<

    "[node0.domain.tld]\n" <<
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
    "threads = 1\n"             <<
    "# The number of queries per thread\n"
    "queries-per-thread = 10000\n"          <<
    "# Duplicates the log to console when threads=1 and workers=1\n"
    "verbose = No\n"             <<
    "# Log all queries\n" <<
    "log-all-queries = No\n"             <<
    "# Log succeeded queries\n" <<
    "log-succeeded-queries = No\n"             <<
    "# Log failed queries\n" <<
    "log-failed-queries = No\n"             <<
    "# Execute SQL randomly\n" <<
    "shuffle = Yes\n"       <<
    "# Extended output of query result\n"
    "log-query-statistics = No\n"             <<
    "# Log query duration in milliseconds\n" <<
    "log-query-duration = No\n"             <<
    "# Log output from executed query (separate log)\n" <<
    "log-client-output = No\n"             <<
    "# Log query numbers along the query results and statistics\n" <<
    "log-query-number = No\n\n" <<
    "[node1.domain.tld]\n" <<
    "address = 10.10.6.10\n" <<
    "# default for \"run\" is No, need to set it explicitly\n" <<
    "run = Yes\n\n" <<
    "[node2.domain.tld]\n" <<
    "address = 10.10.6.11\n" << std::endl;
  }
