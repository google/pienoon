#include "gtest/gtest.h"
#include "entity/vector_pool.h"

class VectorPoolTests : public ::testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
};

// Quick test to iterate over a pool and modify the values.
TEST_F(VectorPoolTests, Iterator_LoopAndModify) {
  int data[] = {1, 2, 3};
  fpl::VectorPool<int> pool;
  *pool.GetNewElement(fpl::kAddToBack) = data[0];
  *pool.GetNewElement(fpl::kAddToBack) = data[1];
  *pool.GetNewElement(fpl::kAddToBack) = data[2];

  int i = 0;
  for (fpl::VectorPool<int>::Iterator it = pool.begin();
       it != pool.end(); ++it) {
    *it += 1;
    EXPECT_EQ(data[i] + 1, *it);
    i++;
  }
}

// Quick test to iterate over a ConstIterator of the pool.
TEST_F(VectorPoolTests, ConstIterator_Loop) {
  int data[] = {1, 2, 3};
  fpl::VectorPool<int> pool;
  *pool.GetNewElement(fpl::kAddToBack) = data[0];
  *pool.GetNewElement(fpl::kAddToBack) = data[1];
  *pool.GetNewElement(fpl::kAddToBack) = data[2];

  int i = 0;
  for (fpl::VectorPool<int>::ConstIterator it = pool.cbegin();
       it != pool.cend(); ++it) {
    EXPECT_EQ(data[i], *it);
    i++;
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
