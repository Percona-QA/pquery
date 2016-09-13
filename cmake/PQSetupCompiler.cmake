IF(CMAKE_SIZEOF_VOID_P EQUAL 8) 
  SET(ARCH "x86_64") 
ELSE() 
  SET(ARCH "i386") 
ENDIF() 
#
MESSAGE(STATUS "Architecture is ${ARCH}")
# c++11 is needed to compile the source, so we're accepting GCC >= 4.7
IF ((${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0"))
    MESSAGE(FATAL_ERROR "Your compiler is too old, please install GCC C++ >= 4.7")
ENDIF ()
#
IF (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
  IF ((${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang") AND (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "7.0.2"))
    MESSAGE(FATAL_ERROR "Your compiler is too old, please install Clang >= 7.0.2")
  ENDIF()
ENDIF()
#
ADD_DEFINITIONS(-std=gnu++11)
#
OPTION(STRICT "Turn on a lot of compiler warnings" ON)
OPTION(DEBUG "Add debug info for GDB" OFF)
OPTION(STATIC_LIB "Statically compile MySQL library into PQuery" ON)
#
IF (DEBUG)
  ADD_DEFINITIONS(-O0 -pipe -g3 -ggdb3)
ELSE()
  ADD_DEFINITIONS(-O3 -march=native -mtune=generic -pipe)
ENDIF ()
#
IF (STRICT)
  ADD_DEFINITIONS(-Wall -Werror -Wextra -pedantic-errors -Wmissing-declarations)
ENDIF ()
#
IF (ASAN)
# doesn't work with GCC < 4.8
  ADD_DEFINITIONS(-fsanitize=address)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
ENDIF()
#
IF (STATIC_LIB)
  # we will link shared libraries
  INCLUDE(FindOpenSSL REQUIRED)
  INCLUDE(FindThreads REQUIRED)
  INCLUDE(FindZLIB REQUIRED)
  # and link static MySQL client library
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  SET (OTHER_LIBS pthread z)
  IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET (OTHER_LIBS ${OTHER_LIBS} dl rt)
  ENDIF()
  IF(PERCONASERVER OR WEBSCALESQL OR PERCONACLUSTER)
    SET(OTHER_LIBS ${OTHER_LIBS} ssl crypto)
  ENDIF(PERCONASERVER OR WEBSCALESQL OR PERCONACLUSTER)
ENDIF(STATIC_LIB)
