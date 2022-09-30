cmake_minimum_required(VERSION 3.21)
project(matching_engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)

# add the executable
add_executable(
        matching_engine
        ./main.cpp
        ./src/clob.cpp
        ./src/matching_engine.cpp
)
