#include "archetype/archetype.h"

DEFINE_ARCHETYPE(writable, (
  (int, write, const char *buf, int size),
));

