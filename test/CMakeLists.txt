# macro expansion tests for gcc, clang, and msvc

add_executable(
  archetype-macro-test
  macro_test.cpp
)

target_include_directories(
  archetype-macro-test
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(
  archetype-macro-test
  PRIVATE cxx_std_11
)

if (MSVC)
  target_compile_options(
    archetype-macro-test 
    PRIVATE /Zc:preprocessor
  )
endif()

# run tests on default build
add_custom_target(
  run-macro-tests ALL
  COMMAND archetype-macro-test 
)




# only running full tests on linux and macos
# macro expansion tests should suffice for windows
if(NOT WIN32)

  find_package(doctest REQUIRED)
  include(doctest)

  add_executable(
    archetype-full-test
    full_test.cpp
  )

  target_include_directories(
    archetype-full-test
    PRIVATE
    ${CMAKE_SOURCE_DIR}/include
  )

  target_compile_options(
    archetype-full-test
    PRIVATE 
    -Wall 
    -Wextra 
    -Werror
  )

  target_compile_features(
    archetype-full-test
    PRIVATE cxx_std_11
  )

  # run tests on default build
  add_custom_target(
    run-tests ALL
    COMMAND archetype-full-test 
  )
endif()
