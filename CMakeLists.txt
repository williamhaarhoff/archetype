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
add_executable(archetype src/example.cpp)

if (MSVC)
  target_compile_options(
   archetype 
    PRIVATE /Zc:preprocessor
  )
endif()


option(TESTING "Enable tests." ON)

if(TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

install(TARGETS archetype DESTINATION bin)
