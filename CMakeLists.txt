cmake_minimum_required(VERSION 3.26)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(archetype
  VERSION 0.0.0
  LANGUAGES CXX)

include_directories(
  include
  src
)

if (NOT WIN32)
  message(STATUS "Building the example!")
  # add_executable(multiinheritance src/multiinheritance.cpp)
  # add_executable(mixin_patterns src/mixin_patterns.cpp)
  add_executable(archetype src/basic_usage.cpp)
  # add_executable(patterns src/patterns.cpp)
  install(TARGETS archetype DESTINATION bin)
endif()


option(TESTING "Enable tests." ON)

if(TESTING)
  enable_testing()
  add_subdirectory(test)
endif()


