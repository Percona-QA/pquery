/*
 =========================================================
 #       Created by Alexey Bychko, Percona LLC           #
 #     Expanded by Roel Van de Paar, Percona LLC         #
 =========================================================
*/

#include <unistd.h>
#include <getopt.h>

#include <INIReader.h>
#include "pquery.hpp"
#include "node.hpp"

std::string confFile = "pquery.cfg";

int
main(int argc, char* argv[]) {

  std::ios_base::sync_with_stdio(false);

  int c;

  while(true) {

    static struct option long_options[] = {
      {"help", no_argument, 0, 'h'},
      {"config-file", required_argument, 0, 'c'},
      {0, 0, 0, 0}
    };

    int option_index = 0;

    c = getopt_long_only(argc, argv, "c:", long_options, &option_index);

    if (c == -1) {
      break;
    }

    switch (c) {
      case 'h':
        show_help();
        exit(EXIT_FAILURE);
      case 'c':
        std::cout << "Config file: " << optarg << std::endl;
        confFile = optarg;
        break;
      default:
        break;
    }
  }                                               //while


if(!confFile.empty()){
  INIReader reader(confFile);
  if (reader.ParseError() < 0) {
    std::cout << "Can't load " << confFile << std::endl;
    exit(1);
  }
  std::vector<std::string> sections;
  sections = reader.GetSections();
  std::vector<std::string>::iterator it;
  for (it = sections.begin(); it != sections.end(); it++){
    std::string secName = *it;
    if(reader.GetBoolean(secName, "run", false)){
      std::shared_ptr<Node> newNode = std::make_shared<Node>();
      newNode->setName(secName);
      newNode->startWork(confFile);
    }
  }
}

  mysql_library_end();

  return EXIT_SUCCESS;
}
