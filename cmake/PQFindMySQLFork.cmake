#############################################################
#  Find mysqlclient                                         #
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.          #
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.  #
#  MYSQL_FOUND       - True if MySQL found.                 #
#############################################################
#
OPTION (MYSQL          "Build PQuery with MySQL support"                  OFF)
OPTION (PERCONASERVER  "Build PQuery with Percona Server support"         OFF)
OPTION (PERCONACLUSTER "Build PQuery with Percona XtraDB Cluster support" OFF)
OPTION (WEBSCALESQL    "Build PQuery with WebScaleSQL support"            OFF)
OPTION (MARIADB        "Build PQuery with MariaDB support"                OFF)
#
# Also use MYSQL for MariaDB, as library names and all locations are the same
#
IF (MYSQL OR MARIADB OR PERCONACLUSTER)
  SET(MYSQL_NAMES mysqlclient mysqlclient_r)
ENDIF(MYSQL OR MARIADB OR PERCONACLUSTER)
#
IF(MYSQL)
  SET(PQUERY_EXT "ms")
  SET(FORK "MySQL")
  ADD_DEFINITIONS(-DFORK="MySQL")
ENDIF(MYSQL)
#
IF(MARIADB)
  SET(PQUERY_EXT "md")
  SET(FORK "MariaDB")
  ADD_DEFINITIONS(-DFORK="MariaDB")
ENDIF(MARIADB)
#
IF (PERCONASERVER)
  SET(MYSQL_NAMES perconaserverclient perconaserverclient_r)
  SET(PQUERY_EXT "ps")
  SET(FORK "Percona Server")
  ADD_DEFINITIONS(-DFORK="Percona-Server")
ENDIF()
#
IF (PERCONACLUSTER)
  SET(PQUERY_EXT "pxc")
  SET(FORK "Percona XtraDB Cluster")
  ADD_DEFINITIONS(-DFORK="Percona-XtraDB-Cluster")
ENDIF()
#
IF (WEBSCALESQL)
  SET(MYSQL_NAMES webscalesqlclient webscalesqlclient_r)
  SET(PQUERY_EXT "ws")
  SET(FORK "WebScaleSQL")
  ADD_DEFINITIONS(-DFORK="WebScaleSQL")
ENDIF()
#
IF("${FORK}" STREQUAL "")
  MESSAGE(FATAL_ERROR "\n* Please set fork to compile with:\n* MYSQL | MARIADB | PERCONASERVER | PERCONACLUSTER| WEBSCALESQL\n")
ENDIF()
#
IF (MYSQL_INCLUDE_DIR)
  # Already in cache, be silent
  SET(MYSQL_FIND_QUIETLY TRUE)
ENDIF (MYSQL_INCLUDE_DIR)
#
IF (BASEDIR)
  MESSAGE(STATUS "* BASEDIR is set, looking for ${FORK} in ${BASEDIR}")
ENDIF()
#
FIND_PATH(MYSQL_INCLUDE_DIR mysql.h
  ${BASEDIR}/include
  ${BASEDIR}/include/mysql
  /usr/local/include/mysql
  /usr/include/mysql
  /usr/local/mysql/include
  )
#
FIND_LIBRARY(MYSQL_LIBRARY
  NAMES ${MYSQL_NAMES}
  PATHS ${BASEDIR}/lib ${BASEDIR}/lib64 /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/i386-linux-gnu /usr/lib64 /usr/local/mysql/lib
  PATH_SUFFIXES mysql
  )
#
IF (MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
  SET(MYSQL_FOUND TRUE)
  SET( MYSQL_LIBRARIES ${MYSQL_LIBRARY} )
ELSE (MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
  SET(MYSQL_FOUND FALSE)
  SET( MYSQL_LIBRARIES )
ENDIF (MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
#
IF (MYSQL_FOUND)
  IF (NOT MYSQL_FIND_QUIETLY)
    MESSAGE(STATUS "* Found ${FORK} library: ${MYSQL_LIBRARY}")
    MESSAGE(STATUS "* Found ${FORK} include directory: ${MYSQL_INCLUDE_DIR}")
  ENDIF (NOT MYSQL_FIND_QUIETLY)
ELSE (MYSQL_FOUND)
  MESSAGE(STATUS "* Looked for ${FORK} libraries named ${MYSQL_NAMES}.")
  MESSAGE(FATAL_ERROR "* Could NOT find ${FORK} library")
ENDIF (MYSQL_FOUND)
#
MARK_AS_ADVANCED(MYSQL_LIBRARY MYSQL_INCLUDE_DIR)
