# c++11 is needed to compile the source, so we're accepting GCC >= 4.7
IF ((CMAKE_CXX_COMPILER_ID STREQUAL GNU) AND
    (CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0"))
  MESSAGE(FATAL_ERROR "Your compiler is too old, please install GCC >= 4.7")    
ENDIF ()
#
ADD_DEFINITIONS(-std=gnu++11)
#
OPTION(STRICT "Turn on a lot of compiler warnings" ON)
OPTION(DEBUG "Add debug info for GDB" OFF)
OPTION(STATIC "Build static PQuery" OFF)
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
IF (STATIC)
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF()
