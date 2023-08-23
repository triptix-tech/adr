#include "gtest/gtest.h"

#include "adr/adr.h"

TEST(adr, simple) { adr::extract("test/Darmstadt.osm.pbf", ""); }