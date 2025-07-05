#include "archetype/archetype.h"


#define CATCH_CONFIG_FAST_COMPILE
#include <catch2/catch.hpp>

SCENARIO("basic math", "[basics]") {
  GIVEN("addition works") {
    WHEN("1 + 1 = 2") {
      REQUIRE(1 + 1 == 2);
    }
  }
}
