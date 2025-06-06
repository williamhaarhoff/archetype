cmake_minimum_required(VERSION 3.26)

project(archetype
  VERSION 0.0.0
  LANGUAGES CXX)

include_directories(
  include
)

add_executable(example src/example.cpp)

option(TESTING "Enable tests." ON)

if(TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

install(TARGETS example DESTINATION bin)
