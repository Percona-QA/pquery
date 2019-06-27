#include "common.hpp"
#include "pquery.hpp"
#include <iostream>

std::vector<Option *> *add_options() {
  std::vector<Option *> *options = new std::vector<Option *>;
  options->resize(Option::MAX);
  Option *opt;

  /* DDL option */
  opt = new Option(Option::BOOL, Option::DDL, "ddl");
  opt->help = "Use DDL in workload";
  opt->setBool(true);
  options->at(Option::DDL) = opt;

  /* Initial Table */
  opt = new Option(Option::INT, Option::TABLE, "table");
  opt->help = "Number of initial tables";
  opt->setInt(10);
  options->at(Option::TABLE) = opt;

  /* SELECT */
  opt = new Option(Option::BOOL, Option::SELECT, "select");
  opt->help = "execute SELECT against tables";
  opt->setInt(0);
  options->at(Option::SELECT) = opt;

  /* Drop column */
  opt = new Option(Option::INT, Option::DROP_COLUMN, "atdcpm");
  opt->help = "Alter table drop column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::DROP_COLUMN) = opt;

  /* Add column */
  opt = new Option(Option::INT, Option::ADD_COLUMN, "atacpm");
  opt->help = "Alter table add column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::ADD_COLUMN) = opt;

  /* Truncate column */
  opt = new Option(Option::INT, Option::TRUNCATE, "ttpm");
  opt->help = "Truncate table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::TRUNCATE) = opt;

  /* Drop and recreate table */
  opt = new Option(Option::INT, Option::DROP_CREATE, "dctpm");
  opt->help = "Drop and recreate table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::DROP_CREATE) = opt;

  /*Encryption table */
  opt = new Option(Option::INT, Option::ENCRYPTION, "atsepm");
  opt->help = "Alter table set Encrytion";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::ENCRYPTION) = opt;

  /*Tablespace Encrytion */
  opt = new Option(Option::INT, Option::TABLESPACE_ENCRYPTION, "asepm");
  opt->help = "Alter tablespace set Encrytion";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();
  options->at(Option::TABLESPACE_ENCRYPTION) = opt;

  /* Tablespace Rename */
  opt = new Option(Option::INT, Option::TABLESPACE_RENAME, "asrpm");
  opt->help = "Alter tablespace rename";
  opt->setInt(1);
  options->at(Option::TABLESPACE_RENAME) = opt;
  opt->setSQL();
  opt->setDDL();

  /* HELP */
  opt = new Option(Option::BOOL, Option::HELP, "help");
  opt->help = "user asked for help";
  opt->setArgs(optional_argument);
  options->at(Option::HELP) = opt;

  /* DATABASE */
  opt = new Option(Option::STRING, Option::DATABASE, "database");
  opt->help = "The database to connect to";
  opt->setString("test");
  options->at(Option::DATABASE) = opt;

  /* Address */
  opt = new Option(Option::STRING, Option::ADDRESS, "address");
  opt->help = "IP address to connect to";
  options->at(Option::ADDRESS) = opt;

  /* Infile */
  opt = new Option(Option::STRING, Option::INFILE, "infile");
  opt->help = "The SQL input file";
  options->at(Option::INFILE) = opt;

  /* Logdir */
  opt = new Option(Option::STRING, Option::LOGDIR, "logdir");
  opt->help = "Log directory";
  options->at(Option::LOGDIR) = opt;

  /* Socket */
  opt = new Option(Option::STRING, Option::SOCKET, "socket");
  opt->help = "Socket file to use";
  opt->setString("/tmp/my.sock");
  options->at(Option::SOCKET) = opt;

  /*config file */
  opt = new Option(Option::STRING, Option::CONFIGFILE, "config-file");
  opt->help = "Config file to use for test";
  options->at(Option::CONFIGFILE) = opt;

  /*Port */
  opt = new Option(Option::INT, Option::PORT, "port");
  opt->help = "Port to use";
  opt->setInt(3306);
  options->at(Option::PORT) = opt;

  /* User*/
  opt = new Option(Option::STRING, Option::USER, "user");
  opt->help = "The MySQL userID to be used";
  opt->setString("root");
  options->at(Option::USER) = opt;

  /* Password*/
  opt = new Option(Option::STRING, Option::PASSWORD, "password");
  opt->help = "The MySQL user's password";
  opt->setString("");
  options->at(Option::PASSWORD) = opt;

  /* Threads */
  opt = new Option(Option::INT, Option::THREADS, "threads");
  opt->help = "The number of threads to use";
  opt->setInt(1);
  options->at(Option::THREADS) = opt;
  return options;
}

Option::~Option() {}

void Option::print_pretty() {
  std::cout << "--" << name << ": " << help << std::endl;
  std::cout << " default value: " << std::endl;
}

std::vector<Option *> *options = add_options();

void delete_options() {
  for (auto &it : *options)
    delete it;
  delete options;
}

void show_help(Option::Opt option) {
  /* Print all avilable options */

  if (option == Option::MAX) {
    print_version();
    for (auto &it : *options) {
      it->print_pretty();
    };
  } else {
    std::cout << " Invalid options " << std::endl;
    std::cout << " use --help --verbose to see all supported options "
              << std::endl;
  }
}

void print_version(void) {
  std::cout << " - PQuery v" << PQVERSION << "-" << PQREVISION
            << " compiled with " << FORK << "-" << mysql_get_client_info()
            << std::endl;
}

void show_help() {
  print_version();
  std::cout << " - pquery supports two different use/configuration modes; "
               "commandline or INI-file. Please see specific help:"
            << std::endl;
  std::cout << "=> pquery --config-help for INI-config help, this mode "
               "supports SINGLE and MULTIPLE node(s)"
            << std::endl;
  std::cout << "=> pquery --cli-help for commandline options, this mode "
               "supports a SINGLE node only"
            << std::endl;
  std::cout << " - For complete help use => pquery  --help --verbose"
            << std::endl;
  std::cout << " - For help on any option => pquery --help=OPTION e.g. \n "
               "            pquery --help=ddl"
            << std::endl;
}


void show_cli_help(void) {
  print_version();
  std::cout << " - General usage: pquery --user=USER --password=PASSWORD "
               "--database=DATABASE"
            << std::endl;
  std::cout << "=> pquery doesn't support multiple nodes when using "
               "commandline options mode!"
            << std::endl;
  std::cout << "---------------------------------------------------------------"
               "--------------------------\n"
            << "| OPTION               | EXPLANATION                           "
               "       | DEFAULT         |\n"
            << "---------------------------------------------------------------"
               "--------------------------\n"
            << "--database             | The database to connect to            "
               "       | \n"
            << "--address              | IP address to connect to              "
               "       | \n"
            << "--port                 | The port to connect to                "
               "       | 3306\n"
            << "--infile               | The SQL input file                    "
               "       | pquery.sql\n"
            << "--logdir               | Log directory                         "
               "       | /tmp\n"
            << "--socket               | Socket file to use                    "
               "       | /tmp/my.sock\n"
            << "--user                 | The MySQL userID to be used           "
               "       | shell user\n"
            << "--password             | The MySQL user's password             "
               "       | <empty>\n"
            << "--threads              | The number of threads to use          "
               "       | 1\n"
            << "--queries-per-thread   | The number of queries per thread      "
               "       | 10000\n"
            << "--verbose              | Duplicates the log to console when "
               "threads=1 | no\n"
            << "--log-all-queries      | Log all queries (succeeded and "
               "failed)       | no\n"
            << "--log-succeeded-queries| Log succeeded queries                 "
               "       | no\n"
            << "--log-failed-queries   | Log failed queries                    "
               "       | no\n"
            << "--no-shuffle           | Execute SQL sequentially              "
               "       | randomly\n"
            << "--log-query-statistics | Extended output of query result       "
               "       | no\n"
            << "--log-query-duration   | Log query duration in milliseconds    "
               "       | no\n"
            << "--test-connection      | Test connection to server and exit    "
               "       | no\n"
            << "--log-query-number     | Write query # to logs                 "
               "       | no\n"
            << "--log-client-output    | Log query output to separate file     "
               "       | no\n"
            << "--ddl		    | USE DDL in command line option           "
               "    | true\n"
            << "---------------------------------------------------------------"
               "--------------------------"
            << std::endl;
}

void show_config_help(void) {

  print_version();

  std::cout << " - Usage: pquery --config-file=pquery.cfg" << std::endl;
  std::cout << " - CLI params has been replaced by config file (INI format)"
            << std::endl;
  std::cout << " - You can redefine any global param=value pair in "
               "host-specific section"
            << std::endl;
  std::cout << "\nConfig example:\n" << std::endl;
  std::cout <<

      "[node0.domain.tld]\n"
            << "# The database to connect to\n"
            << "database = \n"
            << "# IP address to connect to, default is AF_UNIX\n"
            << "address = <empty>\n"
            << "# The port to connect to\n"
            << "port = 3306\n"
            << "# The SQL input file\n"
            << "infile = pquery.sql\n"
            << "# Directory to store logs\n"
            << "logdir = /tmp\n"
            << "# Socket file to use\n"
            << "socket = /tmp/my.sock\n"
            << "# The MySQL userID to be used\n"
            << "user = test\n"
            << "# The MySQL user's password\n"
            << "password = test\n"
            << "# The number of threads to use by worker\n"
            << "threads = 1\n"
            << "# The number of queries per thread\n"
               "queries-per-thread = 10000\n"
            << "# Duplicates the log to console when threads=1 and workers=1\n"
               "verbose = No\n"
            << "# Log all queries\n"
            << "log-all-queries = No\n"
            << "# Log succeeded queries\n"
            << "log-succeeded-queries = No\n"
            << "# Log failed queries\n"
            << "log-failed-queries = No\n"
            << "# Execute SQL randomly\n"
            << "shuffle = Yes\n"
            << "# Extended output of query result\n"
               "log-query-statistics = No\n"
            << "# Log query duration in milliseconds\n"
            << "log-query-duration = No\n"
            << "# Log output from executed query (separate log)\n"
            << "log-client-output = No\n"
            << "# Log query numbers along the query results and statistics\n"
            << "log-query-number = No\n\n"
            << "[node1.domain.tld]\n"
            << "address = 10.10.6.10\n"
            << "# default for \"run\" is No, need to set it explicitly\n"
            << "run = Yes\n\n"
            << "[node2.domain.tld]\n"
            << "address = 10.10.6.11\n"
            << std::endl;
}
