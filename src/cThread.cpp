#include <iostream>
#include <cDbWorker.hpp>

void
DbWorker::workerThread(int number){
  //TODO
  #ifdef DEBUG
  std::cerr << __PRETTY_FUNCTION__ << number << std::endl;
#endif
  std::shared_ptr<Logger> tLogger = std::make_shared<Logger>();
  tLogger->initLogFile(mParams.logdir + "/" + mParams.myName + "_thread-" + std::to_string(number) + ".sql");
}
