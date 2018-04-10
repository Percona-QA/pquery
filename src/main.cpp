#include <cstdlib>
#ifdef DEBUG
#include <iostream>
#endif
#include <cPQuery.hpp>

int
main(int argc, char* argv[]) {
#ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif

  PQuery pqueryMaster = PQuery();

  if(!pqueryMaster.parseCliOptions(argc, argv)) {
    return EXIT_FAILURE;
    }

  return pqueryMaster.run();
  }
