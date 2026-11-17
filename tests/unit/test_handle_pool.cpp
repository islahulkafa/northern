#include "core/memory/handle_pool.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace northern::core;

struct TestResourceTag {};
using ResourceHandle = Handle<TestResourceTag>;

TEST(HandlePoolTest, AllocateAndRetrieve) {
    HandlePool<std::string, TestResourceTag> pool;
    auto h1 = pool.allocate("Texture1");
    auto h2 = pool.allocate("Texture2");

    EXPECT_TRUE(pool.isValid(h1));
    EXPECT_TRUE(pool.isValid(h2));
    EXPECT_EQ(pool.activeCount(), 2U);

    ASSERT_NE(pool.get(h1), nullptr);
    ASSERT_NE(pool.get(h2), nullptr);
    EXPECT_EQ(*pool.get(h1), "Texture1");
    EXPECT_EQ(*pool.get(h2), "Texture2");
}

TEST(HandlePoolTest, ReleaseInvalidatesHandle) {
    HandlePool<int, TestResourceTag> pool;
    auto handle = pool.allocate(42);
    EXPECT_TRUE(pool.isValid(handle));

    EXPECT_TRUE(pool.release(handle));
    EXPECT_FALSE(pool.isValid(handle));
    EXPECT_EQ(pool.get(handle), nullptr);
    EXPECT_EQ(pool.activeCount(), 0U);

    // Re-releasing stale handle returns false
    EXPECT_FALSE(pool.release(handle));
}

TEST(HandlePoolTest, SlotRecyclingWithGenerationIncrement) {
    HandlePool<int, TestResourceTag> pool;
    auto firstHandle = pool.allocate(100);
    EXPECT_EQ(firstHandle.index, 0U);
    EXPECT_EQ(firstHandle.generation, 1U);

    pool.release(firstHandle);

    // Allocate again should reuse slot 0 with generation incremented
    auto recycledHandle = pool.allocate(200);
    EXPECT_EQ(recycledHandle.index, 0U);
    EXPECT_EQ(recycledHandle.generation, 2U);

    // Old handle is rejected, new handle is valid
    EXPECT_FALSE(pool.isValid(firstHandle));
    EXPECT_TRUE(pool.isValid(recycledHandle));
    EXPECT_EQ(*pool.get(recycledHandle), 200);
}
