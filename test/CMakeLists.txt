include(CTest)

find_package(
  Catch2 REQUIRED
)
include(Catch)

# archetype-test 
add_executable(
  archetype-test
  main.cpp
  test.cpp
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

target_link_libraries(
  archetype-test 
  Catch2::Catch2
)

catch_discover_tests(
  archetype-test
)
