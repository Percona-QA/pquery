#include "common.hpp"
#include "pquery.hpp"
#include <iostream>

Opx *options = new Opx;
Ser_Opx *server_options = new Ser_Opx;

/* Process --mso=abc=30=40 to abc,{30,40}*/
void add_server_options(std::string str) {
  auto found = str.find_first_of("=", 0);
  if (found == std::string::npos)
    throw std::runtime_error("Invalid string, " + str);

    std::string name = str.substr(0, found);
    Server_Option *so = new Server_Option(name);
    so->prob = 100;
    server_options->push_back(so);
    str = str.substr(found + 1, str.size());

    found = str.find_first_of("=");
    while (found != std::string::npos) {
      auto val = str.substr(0, found);
      so->values.push_back(val);
      str = str.substr(found + 1, str.size());
      found = str.find_first_of("=");
    }
    /* push the last one */
    so->values.push_back(str);
}

/* process file. and push to mysqld server options */
void add_server_options_file(std::string file_name) {
  std::cout << "file_name " << file_name << std::endl;
  std::ifstream f1;
  f1.open(file_name);
  if (!f1)
    throw std::runtime_error("unable to open " + file_name);
  std::string option;
  while (f1 >> option)
    add_server_options(option);
  f1.close();
}

/* add new options */
inline Option *newOption(Option::Type t, Option::Opt o, std::string s) {
  auto *opt = new Option(t, o, s);
  options->at(o) = opt;
  return opt;
}

/* All available options */
void add_options() {
  options->resize(Option::MAX);
  Option *opt;

  /* Mode of Pquery */
  opt = newOption(Option::BOOL, Option::MODE_OF_PQUERY, "mode");
  opt->help =
      "run load from a SQL file or generate them. If set, then infile will be "
      "processed and queries would be executed in random order. If set false "
      "then sql would be generated based on the option provided";
  opt->setBool(true);

  /* Load metadata */
  opt = newOption(Option::BOOL, Option::METADATA_READ, "metadata-read");
  opt->help = "Load table structures from a json file or created then randomly";
  opt->setBool(false);

  /* write metadata */
  opt = newOption(Option::BOOL, Option::METADATA_WRITE, "metadata-write");
  opt->help = "write metadata to a file. see option --metadata-write-file and "
              "--metadata-read-file";
  opt->setBool(true);

  /* Save metadata to a file */
  opt = newOption(Option::STRING, Option::METADATA_READ_FILE,
                  "metadata-read-file");
  opt->help = "read metadata from file name";
  opt->setString("/tmp/data.dll");

  /* Read metadata from file */
  opt = newOption(Option::STRING, Option::METADATA_WRITE_FILE,
                  "metadata-write-file");
  opt->help = "write metadata to a file, see option --metadata-read, "
              "--metadata-write ";
  opt->setString("/tmp/new_data.dll");

  /* Intial Seed for test */
  opt = newOption(Option::INT, Option::INITIAL_SEED, "seed");
  opt->help = "Initial seed used for the test";
  opt->setInt(1);

  /* Number of General tablespaces */
  opt =
      newOption(Option::INT, Option::NUMBER_OF_GENERAL_TABLESPACE, "tbs-count");
  opt->setInt("1");
  opt->help = "random number of different general tablespaces ";

  /* Engine */
  opt = newOption(Option::STRING, Option::ENGINE, "engine");
  opt->help = "Engine used ";
  opt->setString("INNODB");

  /* Just Load DDL*/
  opt = newOption(Option::BOOL, Option::JUST_LOAD_DDL, "jlddl");
  opt->help = "load DDL and exit";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* DDL option */
  opt = newOption(Option::BOOL, Option::DDL, "ddl");
  opt->help = "Use DDL in workload";
  opt->setBool(true);

  /* disable all type of encrytion */
  opt = newOption(Option::BOOL, Option::NO_ENCRYPTION, "no-encryption");
  opt->help = "Disable All type of encrytion";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* disable all type of encrytion */
  opt = newOption(Option::BOOL, Option::NO_TABLESPACE, "no-tbs");
  opt->help = "disable all type of tablespace including the general tablespace";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* Initial Table */
  opt = newOption(Option::INT, Option::TABLES, "tables");
  opt->help = "Number of initial tables";
  opt->setInt(10);

  /* Number of indexes in a table */
  opt = newOption(Option::INT, Option::INDEXES, "indexes");
  opt->help = "maximum indexes in a table,default depends on page-size as well";
  opt->setInt(7);

  /* Number of columns in a table */
  opt = newOption(Option::INT, Option::COLUMNS, "columns");
  opt->help =
      "maximum columns in a table, default depends on page-size as well";
  opt->setInt(10);

  /* Number of columns in an index of a table */
  opt = newOption(Option::INT, Option::INDEX_COLUMNS, "index-columns");
  opt->help = "maximum columns in an index of a table, default depends on "
              "page-size as well";
  opt->setInt(10);

  /* autoinc column */
  opt = newOption(Option::BOOL, Option::NO_AUTO_INC, "no-auto-inc");
  opt->help = "Disable auto inc columns in table, including pkey";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* desc index support */
  opt = newOption(Option::BOOL, Option::NO_DESC_INDEX, "no-desc-index");
  opt->help = "Disable index with desc on tables ";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* Only Temporary tables */
  opt = newOption(Option::BOOL, Option::ONLY_TEMPORARY, "only-temp-tables");
  opt->help = "Work only on tempoary tables";
  opt->setArgs(no_argument);
  opt->setBool(false);

  /* Initial Records in table */
  opt = newOption(Option::INT, Option::INITIAL_RECORDS_IN_TABLE, "records");
  opt->help = "Number of initial records in table";
  opt->setInt(1000);

  /* Execute workload for number of seconds */
  opt = newOption(Option::INT, Option::NUMBER_OF_SECONDS_WORKLOAD, "seconds");
  opt->help = "Number of seconds to execute workload";
  opt->setInt(1000);

  /* primary key probablity */
  opt = newOption(Option::INT, Option::PRIMARY_KEY, "ctpkp");
  opt->help = "Probability of adding primary key in a table";
  opt->setInt(50);

  /*Encrypt table */
  opt = newOption(Option::INT, Option::ALTER_TABLE_ENCRYPTION, "atsepm");
  opt->help = "Alter table set Encrytion";
  opt->setInt(10);
  opt->setSQL();
  opt->setDDL();

  /* Row Format */
  opt = newOption(Option::STRING, Option::ROW_FORMAT, "row-format");
  opt->help =
      "create table row format. it is  the row format of  table. a "
      "table can have compressed, dynamic, redundant row format.\n "
      "valid values are :\n all: use compressed, dynamic, redundant. all "
      "combination key block size will be used. \n uncompressed: do not use "
      "compressed row_format, i.e. key block size will not used. \n  "
      "none: do not use any encryption";
  opt->setString("all");


  /* MySQL server option */
  opt = newOption(Option::STRING, Option::MYSQLD_SERVER_OPTION, "mso");
  opt->help =
      "mysqld server options variables which are set during the load, see "
      "--set-global. n:option=v1=v2 where n is probabality of picking "
      "option, v1 and v2 different value that is supported. "
      "for e.g. --md=20:innodb_temp_tablespace_encrypt=on=off";

  opt = newOption(Option::STRING, Option::SERVER_OPTION_FILE, "sof");
  opt->help = "server options file, MySQL server options file, picks some of "
              "the mysqld options, "
              "and try to set them during the load , using set global and set "
              "session.\n see --set-global.\n File should contain lines like\n "
              "20:innodb_temp_tablespace_encrypt=on=off\n, means 20% chances "
              "that it would be processed. ";

  /* Set Global */
  opt = newOption(Option::INT, Option::SET_GLOBAL_VARIABLE, "set-global");
  opt->help = "set global variable during the load";
  opt->setInt(3);
  opt->setSQL();
  opt->setDDL();

  /*Tablespace Encrytion */
  opt = newOption(Option::INT, Option::ALTER_TABLESPACE_ENCRYPTION, "asepm");
  opt->help = "Alter tablespace set Encrytion";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Tablespace Rename */
  opt = newOption(Option::INT, Option::ALTER_TABLESPACE_RENAME, "asrpm");
  opt->help = "Alter tablespace rename";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* SELECT */
  opt = newOption(Option::BOOL, Option::SELECT, "select");
  opt->help = "Execute any type select on tables";
  opt->setBool(true);

  /* INSERT */
  opt = newOption(Option::BOOL, Option::INSERT, "insert");
  opt->help = "Execute insert into tables";
  opt->setBool(true);

  /* UPDATE */
  opt = newOption(Option::BOOL, Option::UPDATE, "update");
  opt->help = "Execute any type  of update on tables";
  opt->setBool(true);

  /* DELETE */
  opt = newOption(Option::BOOL, Option::DELETE, "delete");
  opt->help = "Execute any type of  delete on tables";
  opt->setBool(true);

  /* Select all row */
  opt = newOption(Option::INT, Option::SELECT_ALL_ROW, "stapm");
  opt->help = "Selecting Tables All data probablity";
  opt->setInt(80);
  opt->setSQL();

  opt->help = "Select table row using pkey probablity";
  opt->setInt(800);
  opt->setSQL();

  /* Insert random row */
  opt = newOption(Option::INT, Option::INSERT_RANDOM_ROW, "itrpm");
  opt->help = "insert random row";
  opt->setInt(800);
  opt->setSQL();

  /* Update row using pkey */
  opt = newOption(Option::INT, Option::UPDATE_ROW_USING_PKEY, "utppm");
  opt->help = "Update row using pkey";
  opt->setInt(40);
  opt->setSQL();

  /* Delete all rows */
  opt = newOption(Option::INT, Option::DELETE_ALL_ROW, "dtapm");
  opt->help = "delete all rows of a table";
  opt->setInt(1);
  opt->setSQL();

  /* Delete row using pkey */
  opt = newOption(Option::INT, Option::DELETE_ROW_USING_PKEY, "dtppm");
  opt->help = "delete row using pkey";
  opt->setInt(40);
  opt->setSQL();

  /* Drop column */
  opt = newOption(Option::INT, Option::DROP_COLUMN, "atdcpm");
  opt->help = "alter table drop column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Add column */
  opt = newOption(Option::INT, Option::ADD_COLUMN, "atacpm");
  opt->help = "alter table add column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Rename Column */
  opt = newOption(Option::INT, Option::RENAME_COLUMN, "atrcpm");
  opt->help = "alter table rename column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Analyze Table */
  opt = newOption(Option::INT, Option::ANALYZE, "tapm");
  opt->help = "Analyze Table";
  opt->setInt(1);
  opt->setSQL();

  /* Optimize Table */
  opt = newOption(Option::INT, Option::OPTIMIZE, "topm");
  opt->help = "Optimize Table";
  opt->setInt(3);
  opt->setSQL();

  /* Truncate table */
  opt = newOption(Option::INT, Option::TRUNCATE, "ttpm");
  opt->help = "Truncate table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Drop and recreate table */
  opt = newOption(Option::INT, Option::DROP_CREATE, "tdcpm");
  opt->help = "Drop and recreate table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* DATABASE */
  opt = newOption(Option::STRING, Option::DATABASE, "database");
  opt->help = "The database to connect to";
  opt->setString("test");

  /* Address */
  opt = newOption(Option::STRING, Option::ADDRESS, "address");
  opt->help = "IP address to connect to";

  /* Infile */
  opt = newOption(Option::STRING, Option::INFILE, "infile");
  opt->help = "The SQL input file";

  /* Logdir */
  opt = newOption(Option::STRING, Option::LOGDIR, "logdir");
  opt->help = "Log directory";
  opt->setString("/tmp");

  /* Socket */
  opt = newOption(Option::STRING, Option::SOCKET, "socket");
  opt->help = "Socket file to use";
  opt->setString("/tmp/socket.sock");

  /*config file */
  opt = newOption(Option::STRING, Option::CONFIGFILE, "config-file");
  opt->help = "Config file to use for test";

  /*Port */
  opt = newOption(Option::INT, Option::PORT, "port");
  opt->help = "Port to use";
  opt->setInt(3306);

  /* Password*/
  opt = newOption(Option::STRING, Option::PASSWORD, "password");
  opt->help = "The MySQL user's password";
  opt->setString("");

  /* HELP */
  opt = newOption(Option::BOOL, Option::HELP, "help");
  opt->help = "user asked for help";
  opt->setArgs(optional_argument);

  /* Threads */
  opt = newOption(Option::INT, Option::THREADS, "threads");
  opt->help = "The number of threads to use";
  opt->setInt(1);

  /* User*/
  opt = newOption(Option::STRING, Option::USER, "user");
  opt->help = "The MySQL userID to be used";
  opt->setString("root");

  /* log all queries */
  opt = newOption(Option::BOOL, Option::LOG_ALL_QUERIES, "log-all-queries");
  opt->help = "Log all queries (succeeded and failed)";
  opt->setBool(true);
  opt->setArgs(no_argument);

  /* log failed queries */
  opt =
      newOption(Option::BOOL, Option::LOG_FAILED_QUERIES, "log-failed-queries");
  opt->help = "Log all failed queries";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* log success queries */
  opt = newOption(Option::BOOL, Option::LOG_SUCCEDED_QUERIES,
                  "log-succeeded-queries");
  opt->help = "Log succeeded queries";
  opt->setBool(false);
  opt->setArgs(no_argument);
}

Option::~Option() {}

void Option::print_pretty() {
  std::string s;
  s = "--" + name + "      ";
  std::cout << "--" << name << ": " << help << std::endl;
  std::cout << " default";
  switch (type) {
  case STRING:
    std::cout << ": " << getString() << std::endl;
    break;
  case INT:
    std::cout << "#: " << getInt() << std::endl;
    break;
  case BOOL:
    std::cout << ": " << getBool() << std::endl;
  }
}

/* delete options and server options */
void delete_options() {
  for (auto &i : *options)
    delete i;
  delete options;
  /* delete server options */
  for (auto &i : *server_options)
    delete i;
  delete server_options;
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

void show_help(std::string help) {
  if (help.compare("verbose") == 0) {
    for (auto &op : *options) {
      if (op != nullptr)
        op->print_pretty();
    }
  }
  for (auto &op : *options) {
    if (op != nullptr && help.size() > 0 && help.compare(op->getName()) == 0)
      op->print_pretty();
    }
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
    std::cout
        << "---------------------------------------------------------------"
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
    std::cout
        <<

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
