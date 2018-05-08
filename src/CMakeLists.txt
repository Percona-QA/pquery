SET(BINARY_NAME "pquery${vMajor}")
ADD_SUBDIRECTORY(third_party)
INCLUDE_DIRECTORIES(third_party/inih++)
SET(LIBS_TO_LINK ${LIBS_TO_LINK} iniconf)
SET(COMMON_SOURCES
    main.cpp cPQuery.cpp cLogger.cpp cDbWorker.cpp cDatabase.cpp
    )
#
IF(MYSQL_FOUND)
  SET(MYSQL_SOURCES
      cMysqlWorker.cpp
      cMysqlDatabase.cpp
      )
ENDIF(MYSQL_FOUND)
#
IF(PGSQL_FOUND)
  SET(PGSQL_SOURCES
      cPgsqlWorker.cpp
      cPgsqlDatabase.cpp
      )
ENDIF(PGSQL_FOUND)
#
IF(MONGO_FOUND)
  SET(MONGO_SOURCES cMongoWorker.cpp)
ENDIF(MONGO_FOUND)
#
ADD_EXECUTABLE(${BINARY_NAME} ${COMMON_SOURCES}
               ${MYSQL_SOURCES}
               ${PGSQL_SOURCES}
               ${MONGO_SOURCES}
               )
TARGET_LINK_LIBRARIES( ${BINARY_NAME} ${LIBS_TO_LINK} )
INSTALL(TARGETS ${BINARY_NAME} DESTINATION bin)
