SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
ADD_LIBRARY(iniconf STATIC ini.c INIReader.cpp)
