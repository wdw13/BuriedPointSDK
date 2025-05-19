#include "gtest/gtest.h"
#include "include/buried.h"

TEST(BuriedBasicTest, Test1) { Buried_Create("D:\BuriedPointSDK"); }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}