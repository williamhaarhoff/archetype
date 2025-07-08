include(CTest)

find_package(doctest REQUIRED)

include(doctest)

add_executable(
  archetype-test
  main.cpp
)

target_include_directories(
  archetype-test
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(
  archetype-test
  PRIVATE 
  -Wall 
  -Wextra 
  -Werror
)

target_compile_features(
  archetype-test
  PRIVATE cxx_std_11
)

# run tests on default build
add_custom_target(
  run-tests ALL
  COMMAND archetype-test 
)

