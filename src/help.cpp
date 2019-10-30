#include "common.hpp"
#include "pquery.hpp"
#include <iostream>

Opx *options = new Opx;
Ser_Opx *server_options = new Ser_Opx;

/* Process --mso=abc=30=40 to abc,{30,40}*/
void add_server_options(std::string str) {
  auto found = str.find_first_of(":", 0);
  int probablity = 100;
  if (found != std::string::npos) {
    probablity = std::stoi(str.substr(0, found));
    str = str.substr(found + 1, str.size());
  }

  /* extract probability */
  found = str.find_first_of("=", 0);
  if (found == std::string::npos)
    throw std::runtime_error("Invalid string, " + str);

  std::string name = str.substr(0, found);
  Server_Option *so = new Server_Option(name);
  so->prob = probablity;
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
  opt = newOption(Option::BOOL, Option::DYNAMIC_PQUERY, "dynamic-pquery");
  opt->help =
      "run pquery from generator or infine.\n If set, then query will be "
      "dynamically generated using random generator else sqls will be executed "
      "from --infine in some order based on shuffle. you can also use -k";
  opt->setBool(true); // todo disable in release
  opt->setArgs(no_argument);

  /* Intial Seed for test */
  opt = newOption(Option::INT, Option::INITIAL_SEED, "seed");
  opt->help = "Initial seed used for the test";
  opt->setInt(1);

  /* Number of General tablespaces */
  opt =
      newOption(Option::INT, Option::NUMBER_OF_GENERAL_TABLESPACE, "tbs-count");
  opt->setInt("1");
  opt->help = "random number of different general tablespaces ";

  /* Number of Undo tablespaces */
  opt =
      newOption(Option::INT, Option::NUMBER_OF_UNDO_TABLESPACE, "undo-tbs-count");
  opt->setInt("3");
  opt->help = "Number of default undo tablespaces ";

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
  opt = newOption(Option::BOOL, Option::NO_DDL, "noddl");
  opt->help = "do not use ddl in workload";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* Only command line sql option */
  opt = newOption(Option::BOOL, Option::ONLY_CL_SQL, "only-cl-sql");
  opt->help = "only run command line sql. other sql will be disable";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* Only command line ddl option */
  opt = newOption(Option::BOOL, Option::ONLY_CL_DDL, "only-cl-ddl");
  opt->help = "only run command line ddl. other ddl will be disable";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* disable table compression */
  opt = newOption(Option::BOOL, Option::NO_TABLE_COMPRESSION,
                  "no-table-compression");
  opt->help = "Disable table compression";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* disable column compression */
  opt = newOption(Option::BOOL, Option::NO_COLUMN_COMPRESSION,
                  "no-columnn-compression");
  opt->help = "Disable column compression. It is percona style compression";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* disable all type of encrytion */
  opt = newOption(Option::BOOL, Option::NO_ENCRYPTION, "no-encryption");
  opt->help = "Disable All type of encrytion";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* create,alter,drop undo tablespace */
  opt = newOption(Option::INT, Option::UNDO_SQL, "undo-tbs-sql");
  opt->help = "Assign probability of running create/alter/drop undo tablespace";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* disable virtual columns*/
  opt = newOption(Option::BOOL, Option::NO_VIRTUAL_COLUMNS, "no-virtual");
  opt->help = "Disable virtual columns";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* disable blob,text columns*/
  opt = newOption(Option::BOOL, Option::NO_BLOB, "no-blob");
  opt->help = "Disable blob columns";
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

  /* algorithm for alter */
  opt = newOption(Option::STRING, Option::ALGORITHM, "alter-algorith");
  opt->help = "algorithm used in alter table. INPLACE|COPY|DEFAULT\n all "
              "means randomly one of them will be picked";
  opt->setString("all");

  /* lock for alter */
  opt = newOption(Option::STRING, Option::LOCK, "alter-lock");
  opt->help = "lock mechanism used in alter table.\n "
              "DEFAULT|NONE|SHARED|EXCLUSIVE.\n all means randomly one of them "
              "will be picked";
  opt->setString("all");

  /* Number of columns in a table */
  opt = newOption(Option::INT, Option::COLUMNS, "columns");
  opt->help = "maximum columns in a table, default depends on page-size, "
              "branch. for 8.0 it is 7 for 5.7 it 10";
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

  /* NO Temporary tables */
  opt = newOption(Option::BOOL, Option::NO_TEMPORARY, "no-temp-tables");
  opt->help = "do not work on tempoary tables";
  opt->setArgs(no_argument);
  opt->setBool(false);

  /* Ratio of temporary table to normal table */
  opt = newOption(Option::INT, Option::TEMPORARY_TO_NORMAL_RATIO,
                  "ratio-normal-temp");
  opt->help = "ratio of normal to temporary tables. for e.g. if ratio to "
              "normal table to temporary is 10 . --tables 40. them only 4 "
              "temorary table will be created per session";
  opt->setInt(10);

  /* Initial Records in table */
  opt = newOption(Option::INT, Option::INITIAL_RECORDS_IN_TABLE, "records");
  opt->help = "Number of initial records in table";
  opt->setInt(1000);

  /* Execute workload for number of seconds */
  opt = newOption(Option::INT, Option::NUMBER_OF_SECONDS_WORKLOAD, "seconds");
  opt->help = "Number of seconds to execute workload";
  opt->setInt(1000);

  /* primary key probablity */
  opt = newOption(Option::INT, Option::PRIMARY_KEY, "primary-key-probablity");
  opt->help = "Probability of adding primary key in a table";
  opt->setInt(50);

  /*Encrypt table */
  opt = newOption(Option::INT, Option::ALTER_TABLE_ENCRYPTION,
                  "alter-table-encrypt");
  opt->help = "Alter table set Encrytion";
  opt->setInt(10);
  opt->setSQL();
  opt->setDDL();

  /* modify column */
  opt = newOption(Option::INT, Option::ALTER_COLUMN_MODIFY, "modify-column");
  opt->help = "Alter table column modify";
  opt->setInt(10);
  opt->setSQL();
  opt->setDDL();

  /*compress table */
  opt = newOption(Option::INT, Option::ALTER_TABLE_COMPRESSION,
                  "alter-table-compress");
  opt->help = "Alter table compression";
  opt->setInt(10);
  opt->setSQL();
  opt->setDDL();

  /* Row Format */
  opt = newOption(Option::STRING, Option::ROW_FORMAT, "row-format");
  opt->help =
      "create table row format. it is the row format of table. a "
      "table can have compressed, dynamic, redundant row format.\n "
      "valid values are :\n all: use compressed, dynamic, redundant. all "
      "combination key block size will be used. \n uncompressed: do not use "
      "compressed row_format, i.e. key block size will not used. \n"
      "none: do not use any encryption";
  opt->setString("all");


  /* MySQL server option */
  opt = newOption(Option::STRING, Option::MYSQLD_SERVER_OPTION, "mso");
  opt->help =
      "mysqld server options variables which are set during the load, see "
      "--set-variable. n:option=v1=v2 where n is probabality of picking "
      "option, v1 and v2 different value that is supported. "
      "for e.g. --md=20:innodb_temp_tablespace_encrypt=on=off";

  opt = newOption(Option::STRING, Option::SERVER_OPTION_FILE, "sof");
  opt->help =
      "server options file, MySQL server options file, picks some of "
      "the mysqld options, "
      "and try to set them during the load , using set global and set "
      "session.\n see --set-variable.\n File should contain lines like\n "
      "20:innodb_temp_tablespace_encrypt=on=off\n, means 20% chances "
      "that it would be processed. ";

  /* Set Global */
  opt = newOption(Option::INT, Option::SET_GLOBAL_VARIABLE, "set-variable");
  opt->help = "set mysqld variable during the load.(session|global)";
  opt->setInt(3);
  opt->setSQL();
  opt->setDDL();

  /* alter instance rotate innodb master key */
  opt = newOption(Option::INT, Option::ALTER_MASTER_KEY, "rotate-master-key");
  opt->help = "Alter instance rotate innodb master key";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* rotate redo log key */
  opt = newOption(Option::INT, Option::ROTATE_REDO_LOG_KEY,
                  "rotate-redo-log-key");
  opt->help = "Rotate redo log key";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /*Tablespace Encrytion */
  opt = newOption(Option::INT, Option::ALTER_TABLESPACE_ENCRYPTION,
                  "alt-tbs-enc");
  opt->help = "Alter tablespace set Encrytion";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /*Database Encryption */
  opt = newOption(Option::INT, Option::ALTER_DATABASE_ENCRYPTION, "alt-db-enc");
  opt->help = "Alter Database Encryption mode to Y/N";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Tablespace Rename */
  opt =
      newOption(Option::INT, Option::ALTER_TABLESPACE_RENAME, "alt-tbs-rename");
  opt->help = "Alter tablespace rename";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* SELECT */
  opt = newOption(Option::BOOL, Option::NO_SELECT, "no-select");
  opt->help = "do not execute any type select on tables";
  opt->setBool("false");
  opt->setArgs(no_argument);

  /* INSERT */
  opt = newOption(Option::BOOL, Option::NO_INSERT, "no-insert");
  opt->help = "do not execute insert into tables";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* UPDATE */
  opt = newOption(Option::BOOL, Option::NO_UPDATE, "no-update");
  opt->help = "do not execute any type of update on tables";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* DELETE */
  opt = newOption(Option::BOOL, Option::NO_DELETE, "no-delete");
  opt->help = "do not execute any type of delete on tables";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* Select all row */
  opt = newOption(Option::INT, Option::SELECT_ALL_ROW, "select-all-row");
  opt->help = "select all data probablity";
  opt->setInt(8);
  opt->setSQL();

  opt = newOption(Option::INT, Option::SELECT_ROW_USING_PKEY,
                  "select-single-row");
  opt->help = "Select table using single row";
  opt->setInt(800);
  opt->setSQL();

  /* Insert random row */
  opt = newOption(Option::INT, Option::INSERT_RANDOM_ROW, "insert-row");
  opt->help = "insert random row";
  opt->setInt(600);
  opt->setSQL();

  /* Update row using pkey */
  opt =
      newOption(Option::INT, Option::UPDATE_ROW_USING_PKEY, "update-with-cond");
  opt->help = "Update row using using where caluse";
  opt->setInt(200);
  opt->setSQL();

  /* Delete all rows */
  opt = newOption(Option::INT, Option::DELETE_ALL_ROW, "delete-all-row");
  opt->help = "delete all rows of a table";
  opt->setInt(1);
  opt->setSQL();

  /* Delete row using pkey */
  opt =
      newOption(Option::INT, Option::DELETE_ROW_USING_PKEY, "delete-with-cond");
  opt->help = "delete row with where condition";
  opt->setInt(200);
  opt->setSQL();

  /* Drop column */
  opt = newOption(Option::INT, Option::DROP_COLUMN, "drop-column");
  opt->help = "alter table drop some random column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Add column */
  opt = newOption(Option::INT, Option::ADD_COLUMN, "add-column");
  opt->help = "alter table add some random column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Drop index */
  opt = newOption(Option::INT, Option::DROP_INDEX, "drop-index");
  opt->help = "alter table drop random index";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Add column */
  opt = newOption(Option::INT, Option::ADD_INDEX, "add-index");
  opt->help = "alter table add random index";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Rename Column */
  opt = newOption(Option::INT, Option::RENAME_COLUMN, "rename-column");
  opt->help = "alter table rename column";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Analyze Table */
  opt = newOption(Option::INT, Option::ANALYZE, "analyze");
  opt->help = "analyze table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Optimize Table */
  opt = newOption(Option::INT, Option::OPTIMIZE, "optimize");
  opt->help = "optimize table";
  opt->setInt(3);
  opt->setSQL();
  opt->setDDL();

  /* Truncate table */
  opt = newOption(Option::INT, Option::TRUNCATE, "truncate");
  opt->help = "truncate table";
  opt->setInt(1);
  opt->setSQL();
  opt->setDDL();

  /* Drop and recreate table */
  opt = newOption(Option::INT, Option::DROP_CREATE, "recreate-table");
  opt->help = "drop and recreate table";
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
  opt->setString("pquery.sql");

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

  opt = newOption(Option::BOOL, Option::VERBOSE, "verbose");
  opt->help = "verbose";
  opt->setBool(false);
  opt->setArgs(no_argument);

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
  opt->setBool(true); // todo diable while merge
  opt->setArgs(no_argument);

  /* execute sql sequentially */
  opt = newOption(Option::BOOL, Option::NO_SHUFFLE, "no-shuffle");
  opt->help = "execute SQL sequentially | randomly\n";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* log query statistics */
  opt = newOption(Option::BOOL, Option::LOG_QUERY_STATISTICS,
                  "log-query-statistics");
  opt->help = "extended output of query result";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* log client output*/
  opt = newOption(Option::BOOL, Option::LOG_CLIENT_OUTPUT, "log-client-output");
  opt->help = "Log query output to separate file";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* log query number*/
  opt = newOption(Option::BOOL, Option::LOG_QUERY_NUMBERS, "log-query-numbers");
  opt->help = "write query # to logs";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* log query duration */
  opt =
      newOption(Option::BOOL, Option::LOG_QUERY_DURATION, "log-query-duration");
  opt->help = "Log query duration in milliseconds";
  opt->setBool(false);
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

  /* queries per thread */
  opt =
      newOption(Option::INT, Option::QUERIES_PER_THREAD, "queries-per-thread");
  opt->help = "The number of queries per thread";
  opt->setInt(1);

  /* test connection */
  opt = newOption(Option::BOOL, Option::TEST_CONNECTION, "test-connection");
  opt->help = "Test connection to server and exit";
  opt->setBool(false);
  opt->setArgs(no_argument);

  /* transaction probability */
  opt = newOption(Option::INT, Option::TRANSATION_PRB_K, "trx-prb-k");
  opt->help = "probability(out of 1000) of combining sql as single trx";
  opt->setInt(10);

  /* tranasaction size */
  opt = newOption(Option::INT, Option::TRANSACTIONS_SIZE, "trx-size");
  opt->help = "average size of each trx";
  opt->setInt(100);

  /* commit to rollback ratio */
  opt = newOption(Option::INT, Option::COMMMIT_TO_ROLLBACK_RATIO,
                  "commit-rollback-ratio");
  opt->help = "ratio of commit to rollback. e.g.\nif 5, then 5 "
              "transactions will be committed and 1 will be rollback.\n"
              "if 0 then all transactions will be rollback";
  opt->setInt(5);

  /* number of savepoints in trxs */
  opt = newOption(Option::INT, Option::SAVEPOINT_PRB_K, "savepoint-prb-k");
  opt->help = "probability of using savepoint in a transaction.\n Also 25% "
              "such transaction will be rollback to some savepoint";
  opt->setInt(50);

  /* steps */
  opt = newOption(Option::INT, Option::STEP, "step");
  opt->help = "current step in pquery script";
  opt->setInt(1);

  /* metadata file path */
  opt = newOption(Option::STRING, Option::METADATA_PATH, "metadata-path");
  opt->help = "path of metadata file";

  /* sql format for */
  opt = newOption(Option::INT, Option::SPECIAL_SQL, "special-sql");
  opt->help = "special sql";
  opt->setInt(10);
  opt->setSQL();

  /* file name of special sql */
  opt = newOption(Option::STRING, Option::SQL_FILE, "sql-file");
  opt->help =
      "file to be used  for special sql\nT1_INT_1, T1_INT_2 will be replaced "
      "with int columns of some table\n in database T1_VARCHAR_1, T1_VARCHAR_2 "
      "will be replaced with varchar columns of some table in database";
  opt->setString("grammer.sql");
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
  std::cout << std::endl;
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
  bool help_found = false;
  for (auto &op : *options) {
    if (op != nullptr && help.size() > 0 && help.compare(op->getName()) == 0) {
      help_found = true;
      op->print_pretty();
    }
  }
  if (!help_found)
    std::cout << "Not a valid option! " << help << std::endl;
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
