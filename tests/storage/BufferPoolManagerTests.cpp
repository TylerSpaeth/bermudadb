#include "bermudadb/storage/BufferPoolManager.hpp"
#include "bermudadb/storage/DiskManager.hpp"
#include "gtest/gtest.h"
#include <filesystem>
#include <memory>

namespace bermudadb {

class BufferPoolManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger = std::make_unique<Logger>(nullptr);
        // Create a temporary directory for the disk manager
        temp_dir = std::filesystem::temp_directory_path() / "bermudadb_test_dir";
        std::filesystem::create_directories(temp_dir);
        // Point to a file instead of a directory
        disk_manager = std::make_unique<DiskManager>(temp_dir / "test_db.db", *logger);
    }

    void TearDown() override {
        // Clean up the temporary directory
        std::filesystem::remove_all(temp_dir);
    }

    std::filesystem::path temp_dir;
    std::unique_ptr<DiskManager> disk_manager;
    const std::size_t pool_size = 10;
    std::unique_ptr<Logger> logger;
};

// Test basic construction of BufferPoolManager
TEST_F(BufferPoolManagerTest, Construction) {
    EXPECT_NO_THROW({
        BufferPoolManager bpm(pool_size, *disk_manager);
    });
}

// Test creating and fetching a page
TEST_F(BufferPoolManagerTest, CreateAndFetchPage) {
    BufferPoolManager bpm(pool_size, *disk_manager);
    PageId pageId;

    Page* page = bpm.createPage(pageId);
    ASSERT_NE(page, nullptr);

    // Fetching the same page should return the same pointer
    Page* page2 = bpm.fetchPage(pageId);
    EXPECT_EQ(page, page2);

    // Unpin the page (createPage pins it)
    EXPECT_TRUE(bpm.unpinPage(pageId, false));
    // Unpin again (fetchPage pins it)
    EXPECT_TRUE(bpm.unpinPage(pageId, false));
}

// Test unpinning a page
TEST_F(BufferPoolManagerTest, UnpinPage) {
    BufferPoolManager bpm(pool_size, *disk_manager);
    PageId pageId;
    bpm.createPage(pageId); // pinCount = 1

    EXPECT_TRUE(bpm.unpinPage(pageId, false)); // pinCount = 0
    // unpinPage should return false if pinCount is already 0
    EXPECT_FALSE(bpm.unpinPage(pageId, false));
}

// Test deleting a page
TEST_F(BufferPoolManagerTest, DeletePage) {
    BufferPoolManager bpm(pool_size, *disk_manager);
    PageId pageId;
    bpm.createPage(pageId);
    bpm.unpinPage(pageId, false);

    EXPECT_TRUE(bpm.deletePage(pageId));
}

// Test flushing a page
TEST_F(BufferPoolManagerTest, FlushPage) {
    BufferPoolManager bpm(pool_size, *disk_manager);
    PageId pageId;
    bpm.createPage(pageId);

    EXPECT_TRUE(bpm.flushPage(pageId));
    EXPECT_TRUE(bpm.unpinPage(pageId, true));
}

} // namespace bermudadb
