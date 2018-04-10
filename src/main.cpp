#include <cstdlib>
#include <cPQuery.hpp>

int
main(int argc, char* argv[]) {

  PQuery pqueryMaster = PQuery();

  if(!pqueryMaster.parseCliOptions(argc, argv)) {
    return EXIT_FAILURE;
    }

  return pqueryMaster.run();
  }
