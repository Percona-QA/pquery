include_directories(../include)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
add_library(inih++ STATIC ini.c INIReader.cpp)
