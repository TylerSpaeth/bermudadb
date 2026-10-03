#include "bermudadb/storage/DiskManager.hpp"
#include "bermudadb/storage/Page.hpp"

#include "gtest/gtest.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace bermudadb {

    namespace {

        class TestDatabaseFile {
        public:
            explicit TestDatabaseFile(std::string filename)
                : path_(std::move(filename)) {
                std::filesystem::remove(path_);
            }

            ~TestDatabaseFile() {
                std::filesystem::remove(path_);
            }

            const std::filesystem::path& path() const {
                return path_;
            }

        private:
            std::filesystem::path path_;
        };

        void VerifyPageContent(const Page& expected, const Page& actual) {
            for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
                EXPECT_EQ(expected.data[i], actual.data[i])
                    << "Page contents differ at byte " << i;
            }
        }

        Logger logger{nullptr};

    } // namespace


    // -------------------------------------------------------------------------
    // Basic raw I/O
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, BasicReadingAndWriting) {
        TestDatabaseFile file("testfile_basic.db");
        DiskManager disk(file.path(), logger);

        Page writtenPage{};
        writtenPage.data[0] = std::byte{42};
        writtenPage.data[1] = std::byte{100};

        disk.writePage(3, writtenPage);

        Page readPage{};
        disk.readPage(3, readPage);

        VerifyPageContent(writtenPage, readPage);
    }


    TEST(DiskManagerTests, LargePageWrite) {
        TestDatabaseFile file("testfile_large.db");
        DiskManager disk(file.path(), logger);

        Page writtenPage{};

        for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
            writtenPage.data[i] =
                static_cast<std::byte>(i & 0xFF);
        }

        disk.writePage(5, writtenPage);

        Page readPage{};
        disk.readPage(5, readPage);

        VerifyPageContent(writtenPage, readPage);
    }


    TEST(DiskManagerTests, SequentialReadWrite) {
        TestDatabaseFile file("testfile_sequential.db");
        DiskManager disk(file.path(), logger);

        // Do not touch page 0 because it is the allocation bitmap.
        for (PageId pageId = 1; pageId <= 10; ++pageId) {
            Page writePage{};

            for (auto& byte : writePage.data) {
                byte = static_cast<std::byte>(pageId & 0xFF);
            }

            disk.writePage(pageId, writePage);
        }

        // Read them separately so we're testing that writing later pages
        // didn't corrupt earlier ones.
        for (PageId pageId = 1; pageId <= 10; ++pageId) {
            Page readPage{};
            disk.readPage(pageId, readPage);

            for (const auto byte : readPage.data) {
                EXPECT_EQ(
                    byte,
                    static_cast<std::byte>(pageId & 0xFF)
                );
            }
        }
    }


    // -------------------------------------------------------------------------
    // Allocation
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, FirstAllocatedPageIsPageOne) {
        TestDatabaseFile file("testfile_allocate_first.db");
        DiskManager disk(file.path(), logger);

        EXPECT_EQ(disk.allocatePage(), 1);
    }


    TEST(DiskManagerTests, PagesAreAllocatedSequentially) {
        TestDatabaseFile file("testfile_allocate_sequential.db");
        DiskManager disk(file.path(), logger);

        EXPECT_EQ(disk.allocatePage(), 1);
        EXPECT_EQ(disk.allocatePage(), 2);
        EXPECT_EQ(disk.allocatePage(), 3);
        EXPECT_EQ(disk.allocatePage(), 4);
        EXPECT_EQ(disk.allocatePage(), 5);
    }


    TEST(DiskManagerTests, FreedPageIsReused) {
        TestDatabaseFile file("testfile_free_reuse.db");
        DiskManager disk(file.path(), logger);

        const PageId page1 = disk.allocatePage();
        const PageId page2 = disk.allocatePage();
        const PageId page3 = disk.allocatePage();

        ASSERT_EQ(page1, 1);
        ASSERT_EQ(page2, 2);
        ASSERT_EQ(page3, 3);

        ASSERT_TRUE(disk.freePage(page2));

        // Allocation scans from the beginning, so page 2 should be found first.
        EXPECT_EQ(disk.allocatePage(), page2);
    }


    TEST(DiskManagerTests, FreedFirstPageIsReused) {
        TestDatabaseFile file("testfile_free_first.db");
        DiskManager disk(file.path(), logger);

        EXPECT_EQ(disk.allocatePage(), 1);
        EXPECT_EQ(disk.allocatePage(), 2);
        EXPECT_EQ(disk.allocatePage(), 3);

        ASSERT_TRUE(disk.freePage(1));

        EXPECT_EQ(disk.allocatePage(), 1);
    }


    TEST(DiskManagerTests, FreeingBitmapPageFails) {
        TestDatabaseFile file("testfile_free_bitmap.db");
        DiskManager disk(file.path(), logger);

        EXPECT_FALSE(disk.freePage(0));
        EXPECT_FALSE(disk.freePage(PAGES_PER_REGION));
        EXPECT_FALSE(disk.freePage(PAGES_PER_REGION * 2));
    }


    // -------------------------------------------------------------------------
    // Persistence
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, AllocationStatePersistsAfterReopening) {
        TestDatabaseFile file("testfile_allocation_persistence.db");

        {
            DiskManager disk(file.path(), logger);

            EXPECT_EQ(disk.allocatePage(), 1);
            EXPECT_EQ(disk.allocatePage(), 2);
            EXPECT_EQ(disk.allocatePage(), 3);
        }

        {
            DiskManager disk(file.path(), logger);

            EXPECT_EQ(disk.allocatePage(), 4);
        }
    }


    TEST(DiskManagerTests, FreedPagePersistsAfterReopening) {
        TestDatabaseFile file("testfile_free_persistence.db");

        {
            DiskManager disk(file.path(), logger);

            EXPECT_EQ(disk.allocatePage(), 1);
            EXPECT_EQ(disk.allocatePage(), 2);
            EXPECT_EQ(disk.allocatePage(), 3);

            ASSERT_TRUE(disk.freePage(2));
        }

        {
            DiskManager disk(file.path(), logger);

            EXPECT_EQ(disk.allocatePage(), 2);
        }
    }


    TEST(DiskManagerTests, PageContentsPersistAfterReopening) {
        TestDatabaseFile file("testfile_page_persistence.db");

        Page expected{};

        for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
            expected.data[i] =
                static_cast<std::byte>((i * 17) & 0xFF);
        }

        {
            DiskManager disk(file.path(), logger);
            disk.writePage(10, expected);
        }

        {
            DiskManager disk(file.path(), logger);

            Page actual{};
            disk.readPage(10, actual);

            VerifyPageContent(expected, actual);
        }
    }


    // -------------------------------------------------------------------------
    // Bitmap boundary
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, AllocationSkipsSecondBitmapPage) {
        TestDatabaseFile file("testfile_bitmap_boundary.db");
        DiskManager disk(file.path(), logger);

        // Region 0 consists of:
        //
        // Page 0                    -> bitmap
        // Page 1 ... N             -> data
        // Page PAGES_PER_REGION     -> next bitmap
        //
        // Allocate every data page belonging to region 0.
        const PageId dataPagesInRegion = PAGES_PER_REGION - 1;

        PageId lastPage = 0;

        for (PageId i = 0; i < dataPagesInRegion; ++i) {
            lastPage = disk.allocatePage();
        }

        EXPECT_EQ(lastPage, PAGES_PER_REGION - 1);

        const PageId firstPageInSecondRegion = disk.allocatePage();

        // PAGES_PER_REGION itself is the second bitmap page, so allocation
        // must skip it.
        EXPECT_EQ(
            firstPageInSecondRegion,
            PAGES_PER_REGION + 1
        );
    }


    TEST(DiskManagerTests, PageAtEndOfBitmapCanBeFreedAndReused) {
        TestDatabaseFile file("testfile_bitmap_last_bit.db");
        DiskManager disk(file.path(), logger);

        const PageId dataPagesInRegion = PAGES_PER_REGION - 1;

        PageId lastPage = 0;

        for (PageId i = 0; i < dataPagesInRegion; ++i) {
            lastPage = disk.allocatePage();
        }

        ASSERT_EQ(lastPage, PAGES_PER_REGION - 1);

        ASSERT_TRUE(disk.freePage(lastPage));

        EXPECT_EQ(disk.allocatePage(), lastPage);
    }


    // -------------------------------------------------------------------------
    // Multiple DiskManager objects
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, SeparateDiskManagersUseSeparateFiles) {
        TestDatabaseFile file1("testfile_multi1.db");
        TestDatabaseFile file2("testfile_multi2.db");

        DiskManager disk1(file1.path(), logger);
        DiskManager disk2(file2.path(), logger);

        EXPECT_EQ(disk1.allocatePage(), 1);
        EXPECT_EQ(disk2.allocatePage(), 1);

        EXPECT_EQ(disk1.allocatePage(), 2);

        // disk2's allocation state must remain independent.
        EXPECT_EQ(disk2.allocatePage(), 2);
    }


    // -------------------------------------------------------------------------
    // Concurrency
    // -------------------------------------------------------------------------

    TEST(DiskManagerTests, ConcurrentReadsSamePage) {
        TestDatabaseFile file("testfile_concurrent_read.db");
        DiskManager disk(file.path(), logger);

        constexpr PageId pageId = 1;

        Page initialPage{};
        initialPage.data[0] = std::byte{1};

        disk.writePage(pageId, initialPage);

        constexpr int numThreads = 20;

        std::vector<std::thread> threads;
        std::atomic<bool> success{true};

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&]() {
                Page page{};
                disk.readPage(pageId, page);

                if (page.data[0] != std::byte{1}) {
                    success = false;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        EXPECT_TRUE(success);
    }


    TEST(DiskManagerTests, ConcurrentWritesSamePageProduceCompletePage) {
        TestDatabaseFile file("testfile_concurrent_write.db");
        DiskManager disk(file.path(), logger);

        constexpr PageId pageId = 1;
        constexpr int numThreads = 10;

        std::vector<std::thread> threads;

        for (int value = 0; value < numThreads; ++value) {
            threads.emplace_back([&, value]() {
                Page page{};

                for (auto& byte : page.data) {
                    byte = static_cast<std::byte>(value);
                }

                disk.writePage(pageId, page);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        Page finalPage{};
        disk.readPage(pageId, finalPage);

        const auto finalValue =
            static_cast<std::uint8_t>(finalPage.data[0]);

        ASSERT_LT(finalValue, numThreads);

        // The entire page should come from one writer. We don't care which
        // writer won, only that writes weren't interleaved.
        for (const auto byte : finalPage.data) {
            EXPECT_EQ(
                static_cast<std::uint8_t>(byte),
                finalValue
            );
        }
    }


    TEST(DiskManagerTests, ConcurrentAllocationsProduceUniquePageIds) {
        TestDatabaseFile file("testfile_concurrent_allocate.db");
        DiskManager disk(file.path(), logger);

        constexpr int numThreads = 20;

        std::vector<PageId> results(numThreads);
        std::vector<std::thread> threads;

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&, i]() {
                results[i] = disk.allocatePage();
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        std::sort(results.begin(), results.end());

        for (int i = 0; i < numThreads; ++i) {
            EXPECT_EQ(
                results[i],
                static_cast<PageId>(i + 1)
            );
        }
    }

} // namespace bermudadb