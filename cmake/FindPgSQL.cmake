# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindPgSQL
# --------------
#
# Find the PostgreSQL installation.
#
# This module defines
#
# ::
#
#   PGSQL_LIBRARIES - the PostgreSQL libraries needed for linking
#   PGSQL_INCLUDE_DIRS - the directories of the PostgreSQL headers
#   PGSQL_LIBRARY_DIRS  - the link directories for PostgreSQL libraries
#   PGSQL_VERSION_STRING - the version of PostgreSQL found (since CMake 2.8.8)

# ----------------------------------------------------------------------------
# History:
# This module is derived from the module originally found in the VTK source tree.
#
# ----------------------------------------------------------------------------
# Note:
# PGSQL_ADDITIONAL_VERSIONS is a variable that can be used to set the
# version number of the implementation of PostgreSQL.
# In Windows the default installation of PostgreSQL uses that as part of the path.
# E.g C:\Program Files\PostgreSQL\8.4.
# Currently, the following version numbers are known to this module:
# "10" "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0" "8.4" "8.3" "8.2" "8.1" "8.0"
#
# To use this variable just do something like this:
# set(PGSQL_ADDITIONAL_VERSIONS "9.2" "8.4.4")
# before calling find_package(PostgreSQL) in your CMakeLists.txt file.
# This will mean that the versions you set here will be found first in the order
# specified before the default ones are searched.
#
# ----------------------------------------------------------------------------
# You may need to manually set:
#  PGSQL_INCLUDE_DIR  - the path to where the PostgreSQL include files are.
#  PGSQL_LIBRARY_DIR  - The path to where the PostgreSQL library files are.
# If FindPostgreSQL.cmake cannot find the include files or the library files.
#
# ----------------------------------------------------------------------------
# The following variables are set if PostgreSQL is found:
#  PGSQL_FOUND         - Set to true when PostgreSQL is found.
#  PGSQL_INCLUDE_DIRS  - Include directories for PostgreSQL
#  PGSQL_LIBRARY_DIRS  - Link directories for PostgreSQL libraries
#  PGSQL_LIBRARIES     - The PostgreSQL libraries.
#
# ----------------------------------------------------------------------------
# If you have installed PostgreSQL in a non-standard location.
# (Please note that in the following comments, it is assumed that <Your Path>
# points to the root directory of the include directory of PostgreSQL.)
# Then you have three options.
# 1) After CMake runs, set PGSQL_INCLUDE_DIR to <Your Path>/include and
#    PGSQL_LIBRARY_DIR to wherever the library pq (or libpq in windows) is
# 2) Use CMAKE_INCLUDE_PATH to set a path to <Your Path>/PostgreSQL<-version>. This will allow find_path()
#    to locate PGSQL_INCLUDE_DIR by utilizing the PATH_SUFFIXES option. e.g. In your CMakeLists.txt file
#    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "<Your Path>/include")
# 3) Set an environment variable called ${PGSQL_ROOT} that points to the root of where you have
#    installed PostgreSQL, e.g. <Your Path>.
#
# ----------------------------------------------------------------------------
IF(PGSQL_BASEDIR)
  IF (NOT EXISTS ${PGSQL_BASEDIR})
    MESSAGE(FATAL_ERROR "Directory ${PGSQL_BASEDIR} doesn't exist. Check the path for typos!")
  ENDIF(NOT EXISTS ${PGSQL_BASEDIR})
  MESSAGE(STATUS "PGSQL_BASEDIR is set, looking for PostgreSQL in ${PGSQL_BASEDIR}")
ENDIF()
#
IF(PGSQL_INCLUDE_DIR)
  # Already in cache, be silent
  SET(PGSQL_FIND_QUIETLY TRUE)
ENDIF (PGSQL_INCLUDE_DIR)
#
IF(STATIC_PGSQL)
  SET(_pgsql_ORIG_CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES})
  SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
ENDIF(STATIC_PGSQL)
#
SET(PGSQL_KNOWN_VERSIONS ${PGSQL_ADDITIONAL_VERSIONS}
    "10" "9.6" "9.5" "9.4" "9.3" "9.2" "9.1" "9.0" "8.4" "8.3" "8.2" "8.1" "8.0")
#
# Define additional search paths for root directories.
SET( PGSQL_ROOT_DIRECTORIES
   ENV PGSQL_ROOT
   ${PGSQL_ROOT}
   ${PGSQL_BASEDIR}
)

FOREACH(SUFFIX ${PGSQL_KNOWN_VERSIONS})
    LIST(APPEND PGSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES
        "pgsql-${suffix}/lib")
    LIST(APPEND PGSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES
        "pgsql-${suffix}/include")
    LIST(APPEND PGSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES
        "postgresql/${suffix}/server"
        "pgsql-${suffix}/include/server")
ENDFOREACH()

#
# Look for an installation.
#
FIND_PATH(PGSQL_INCLUDE_DIR
  NAMES libpq-fe.h
  PATHS
  /usr/include
  /usr/local/include
  /usr/local/opt

  # Look in other places.
   ${PGSQL_ROOT_DIRECTORIES}
  PATH_SUFFIXES
    pgsql
    postgresql
    include
    ${PGSQL_INCLUDE_ADDITIONAL_SEARCH_SUFFIXES}
  # Help the user find it if we cannot.
  DOC "The ${PGSQL_INCLUDE_DIR_MESSAGE}"
)

FIND_PATH(PGSQL_TYPE_INCLUDE_DIR
  NAMES catalog/pg_type.h
  PATHS
   # Look in other places.
   ${PGSQL_ROOT_DIRECTORIES}
  PATH_SUFFIXES
    postgresql
    pgsql/server
    postgresql/server
    include/server
    ${PGSQL_TYPE_ADDITIONAL_SEARCH_SUFFIXES}
  # Help the user find it if we cannot.
  DOC "The ${PGSQL_INCLUDE_DIR_MESSAGE}"
)

# The PostgreSQL library.
SET (PGSQL_LIBRARY_TO_FIND pq)

FIND_LIBRARY(PGSQL_LIBRARY
 NAMES ${PGSQL_LIBRARY_TO_FIND}
 PATHS
   ${PGSQL_ROOT_DIRECTORIES}
 PATH_SUFFIXES
   lib
   ${PGSQL_LIBRARY_ADDITIONAL_SEARCH_SUFFIXES}
 # Help the user find it if we cannot.
 DOC "The ${PGSQL_LIBRARY_DIR_MESSAGE}"
)

GET_FILENAME_COMPONENT(PGSQL_LIBRARY_DIR ${PGSQL_LIBRARY} PATH)

if (PGSQL_INCLUDE_DIR)
  # Some platforms include multiple pg_config.hs for multi-lib configurations
  # This is a temporary workaround.  A better solution would be to compile
  # a dummy c file and extract the value of the symbol.
  file(GLOB _PG_CONFIG_HEADERS "${PGSQL_INCLUDE_DIR}/pg_config*.h")
  foreach(_PG_CONFIG_HEADER ${_PG_CONFIG_HEADERS})
    if(EXISTS "${_PG_CONFIG_HEADER}")
      file(STRINGS "${_PG_CONFIG_HEADER}" PGSQL_version_str
           REGEX "^#define[\t ]+PG_VERSION[\t ]+\".*\"")
      if(PGSQL_version_str)
        string(REGEX REPLACE "^#define[\t ]+PG_VERSION[\t ]+\"([^\"]*)\".*"
               "\\1" PGSQL_VERSION_STRING "${PGSQL_version_str}")
        break()
      endif()
    endif()
  endforeach()
  unset(PGSQL_version_str)
endif()

# Did we find anything?
#INCLUDE(FindPackageHandleStandardArgs)
#find_package_handle_standard_args(PostgreSQL
#                                  REQUIRED_VARS PGSQL_LIBRARY PGSQL_INCLUDE_DIR PGSQL_TYPE_INCLUDE_DIR
#                                   VERSION_VAR PGSQL_VERSION_STRING)
IF(PGSQL_LIBRARY AND PGSQL_INCLUDE_DIR)
  SET(PGSQL_FOUND TRUE)
  MESSAGE(STATUS "Found PostgreSQL: ${PGSQL_LIBRARY} (found version \"${PGSQL_VERSION_STRING}\")")
  MESSAGE(STATUS "Found PostgreSQL include directory: ${PGSQL_INCLUDE_DIR}")
ENDIF(PGSQL_LIBRARY AND PGSQL_INCLUDE_DIR)

# Now try to get the include and library path.
if(PGSQL_FOUND)
  set(PGSQL_INCLUDE_DIRS ${PGSQL_INCLUDE_DIR} ${PGSQL_TYPE_INCLUDE_DIR} )
  set(PGSQL_LIBRARY_DIRS ${PGSQL_LIBRARY_DIR} )
  set(PGSQL_LIBRARIES ${PGSQL_LIBRARY})
endif()

mark_as_advanced(PGSQL_INCLUDE_DIR PGSQL_TYPE_INCLUDE_DIR PGSQL_LIBRARY )
