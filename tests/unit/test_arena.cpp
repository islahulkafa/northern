#include "core/memory/arena.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace northern::core;

namespace {
struct TestObject {
    int a;
    float b;
    std::string name;

    TestObject(int a_, float b_, std::string name_)
        : a(a_), b(b_), name(std::move(name_)) {}
};
} // namespace

TEST(ArenaTest, BasicAllocationAndAlignment) {
    Arena arena(1024); // 1 KB chunks
    EXPECT_EQ(arena.totalAllocated(), 0);

    void* ptr1 = arena.allocate(100, 16);
    ASSERT_NE(ptr1, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr1) % 16, 0);

    void* ptr2 = arena.allocate(50, 32);
    ASSERT_NE(ptr2, nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(ptr2) % 32, 0);

    EXPECT_GT(arena.totalAllocated(), 150);
}

TEST(ArenaTest, InPlaceObjectCreation) {
    Arena arena(1024);
    TestObject* obj = arena.create<TestObject>(42, 3.14f, "NorthernArena");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->a, 42);
    EXPECT_FLOAT_EQ(obj->b, 3.14f);
    EXPECT_EQ(obj->name, "NorthernArena");
}

TEST(ArenaTest, MultipleChunksAndReset) {
    Arena arena(512); // Small chunks
    // Allocate more than 512 bytes
    void* p1 = arena.allocate(400);
    void* p2 = arena.allocate(400);
    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    EXPECT_GE(arena.chunkCount(), 2);

    size_t allocatedBefore = arena.totalAllocated();
    EXPECT_GE(allocatedBefore, 800);

    arena.reset();
    EXPECT_EQ(arena.totalAllocated(), 0);
    // Chunks should remain allocated in memory pool
    EXPECT_GE(arena.chunkCount(), 2);
}

TEST(ArenaTest, ScopedArenaRollback) {
    Arena arena(1024);
    void* base = arena.allocate(64);
    ASSERT_NE(base, nullptr);
    size_t initialAlloc = arena.totalAllocated();

    {
        ScopedArena scope(arena);
        void* temp1 = arena.allocate(128);
        void* temp2 = arena.allocate(256);
        ASSERT_NE(temp1, nullptr);
        ASSERT_NE(temp2, nullptr);
        EXPECT_GT(arena.totalAllocated(), initialAlloc);
    }

    // After exiting ScopedArena, allocations should be rolled back to initialAlloc
    EXPECT_EQ(arena.totalAllocated(), initialAlloc);
}

TEST(ArenaTest, StlContainerCompatibility) {
    Arena arena(4096);
    using ArenaVec = std::vector<int, ArenaAllocator<int>>;

    ArenaVec vec{ArenaAllocator<int>(arena)};
    for (int i = 0; i < 100; ++i) {
        vec.push_back(i * 2);
    }

    EXPECT_EQ(vec.size(), 100);
    EXPECT_EQ(vec[0], 0);
    EXPECT_EQ(vec[99], 198);
    EXPECT_GT(arena.totalAllocated(), 100 * sizeof(int));
}
