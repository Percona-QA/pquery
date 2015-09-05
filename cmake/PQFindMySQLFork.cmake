#############################################################
#  Find mysqlclient                                         #
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.          #
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.  #
#  MYSQL_FOUND       - True if MySQL found.                 #
#############################################################
#
OPTION (MYSQL         "Build PQuery with MySQL support" OFF)
OPTION (PERCONASERVER "Build PQuery with Percona Server support" OFF)
OPTION (WEBSCALESQL   "Build PQuery with WebScaleSQL support" OFF)
OPTION (MARIADB       "Build PQuery with MariaDB support" OFF)
#
# Also use MYSQL for MariaDB, as library names and all locations are the same
#
IF (MYSQL OR MARIADB)
  SET(MYSQL_NAMES mysqlclient mysqlclient_r)
  if(MYSQL)
    SET(PQUERY_EXT "ms")
    SET(FORK "MySQL")
  ENDIF(MYSQL)
  IF(MARIADB)
    SET(PQUERY_EXT "md")
    SET(FORK "MariaDB")
  ENDIF(MARIADB)
ENDIF(MYSQL OR MARIADB)
#
IF (PERCONASERVER)
  SET(MYSQL_NAMES perconaserverclient perconaserverclient_r)
  SET(PQUERY_EXT "ps")
  SET(FORK "Percona Server")
ENDIF()
#
IF (WEBSCALESQL)
  SET(MYSQL_NAMES webscalesqlclient webscalesqlclient_r)
  SET(PQUERY_EXT "ws")
  SET(FORK "WebScaleSQL")
ENDIF()
#
IF (MYSQL_INCLUDE_DIR)
  # Already in cache, be silent
  SET(MYSQL_FIND_QUIETLY TRUE)
ENDIF (MYSQL_INCLUDE_DIR)
#
FIND_PATH(MYSQL_INCLUDE_DIR mysql.h
  /usr/local/include/mysql
  /usr/include/mysql
  )
#
FIND_LIBRARY(MYSQL_LIBRARY
  NAMES ${MYSQL_NAMES}
  PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/i386-linux-gnu /usr/lib64 
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
    MESSAGE(STATUS "Found ${FORK}: ${MYSQL_LIBRARY}")
  ENDIF (NOT MYSQL_FIND_QUIETLY)
ELSE (MYSQL_FOUND)
  MESSAGE(STATUS "Looked for ${FORK} libraries named ${MYSQL_NAMES}.")
  MESSAGE(FATAL_ERROR "Could NOT find ${FORK} library")
ENDIF (MYSQL_FOUND)
#
MARK_AS_ADVANCED(MYSQL_LIBRARY MYSQL_INCLUDE_DIR)
