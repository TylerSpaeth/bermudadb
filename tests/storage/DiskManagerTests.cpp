#include "bermudadb/storage/DiskManager.hpp"
#include "bermudadb/storage/page.hpp"
#include "gtest/gtest.h"

namespace bermudadb {

    // Helper to verify all bytes in a page
    static void VerifyPageContent(const Page& expected, const Page& actual) {
        for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
            EXPECT_EQ(expected.data[i], actual.data[i]);
        }
    }

    TEST(DiskManagerTests, BasicReadingAndWriting) {
        // Test basic single page write/read with small data
        DiskManager disk("testfile_basic.txt");

        Page writtenPage{};
        writtenPage.data[0] = static_cast<std::byte>(42);
        writtenPage.data[1] = static_cast<std::byte>(100);

        // Write to page 3 (nonexistent file should auto-create)
        disk.write_page(3, writtenPage);

        Page readPage{};
        disk.read_page(3, readPage);

        EXPECT_EQ(writtenPage.data[0], readPage.data[0]);
        EXPECT_EQ(writtenPage.data[1], readPage.data[1]);
    }

    TEST(DiskManagerTests, LargePageWrite) {
        // Verify we can write larger data patterns
        DiskManager disk("testfile_large.txt");

        Page writtenPage{};
        // Fill with alternating pattern to catch corruption
        for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
            writtenPage.data[i] = static_cast<std::byte>(static_cast<int>(i & 15));
        }

        disk.write_page(5, writtenPage);
        Page readPage{};
        disk.read_page(5, readPage);

        VerifyPageContent(writtenPage, readPage);
    }

    TEST(DiskManagerTests, SequentialReadWrite) {
        // Test writing and reading multiple consecutive pages
        DiskManager disk("testfile_sequential.txt");

        for (std::uint64_t pageId = 0; pageId < 10; ++pageId) {
            Page writePage{};
            for (auto& byte : writePage.data) {
                byte = static_cast<std::byte>(static_cast<int>(pageId & 255));
            }

            disk.write_page(pageId, writePage);

            Page readPage{};
            disk.read_page(pageId, readPage);

            EXPECT_EQ(writePage.data[0], readPage.data[0]);
        }
    }

    TEST(DiskManagerTests, PageZeroBoundary) {
        // Verify page 0 works correctly (common edge case)
        DiskManager disk("testfile_zero.txt");

        Page writePage{};
        for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
            writePage.data[i] = static_cast<std::byte>(static_cast<int>('A' + i % 26));
        }

        disk.write_page(0, writePage);
        Page readPage{};
        disk.read_page(0, readPage);

        VerifyPageContent(writePage, readPage);
    }

    TEST(DiskManagerTests, MultipleDiskManagerInstances) {
        // Test that separate instances can coexist with different files
        DiskManager disk1("testfile_multi1.txt");
        DiskManager disk2("testfile_multi2.txt");

        Page page{};
        page.data[0] = static_cast<std::byte>(99);

        disk1.write_page(1, page);
        disk2.write_page(1, page);

        // Verify each manager reads its own file correctly
        Page readPage1{}, readPage2{};
        disk1.read_page(1, readPage1);
        disk2.read_page(1, readPage2);

        EXPECT_EQ(readPage1.data[0], static_cast<std::byte>(99));
        EXPECT_EQ(readPage2.data[0], static_cast<std::byte>(99));
    }

    TEST(DiskManagerTests, ConcurrentReadsSamePage) {
        DiskManager disk("test_concurrent_read.txt");
        const uint64_t pageId = 1;
        Page initialPage{};
        initialPage.data[0] = static_cast<std::byte>(1);
        disk.write_page(pageId, initialPage);

        const int numThreads = 20;
        std::vector<std::thread> threads;
        std::atomic<bool> success{true};

        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&]() {
                Page p{};
                disk.read_page(pageId, p);
                if (p.data[0] != static_cast<std::byte>(1)) {
                    success = false;
                }
            });
        }

        for (auto& t : threads) t.join();
        EXPECT_TRUE(success) << "Data was corrupted or misread during concurrent access.";
    }

    TEST(DiskManagerTests, ConcurrentWritesSamePage) {
        DiskManager disk("test_concurrent_write.txt");
        const uint64_t pageId = 1;
        const int numThreads = 10;

        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([&, i]() {
                Page p{};
                p.data[0] = static_cast<std::byte>(i);
                disk.write_page(pageId, p);
            });
        }

        for (auto& t : threads) t.join();

        // Final state should be one of the valid values, not corrupted garbage
        Page finalP{};
        disk.read_page(pageId, finalP);
        EXPECT_GE(static_cast<uint8_t>(finalP.data[0]), 0);
        EXPECT_LT(static_cast<uint8_t>(finalP.data[0]), numThreads);
    }

    TEST(DiskManagerTests, MixedReadWriteStress) {
        DiskManager disk("test_mixed_stress.txt");
        const uint64_t pageId = 5;
        std::atomic<bool> running{true};

        // Writer thread
        std::thread writer([&]() {
            for (int i = 0; i < 100; ++i) {
                Page p{};
                p.data[0] = static_cast<std::byte>(i % 256);
                disk.write_page(pageId, p);
                std::this_thread::yield();
            }
            running = false;
        });

        // Reader threads
        std::vector<std::thread> readers;
        for (int i = 0; i < 5; ++i) {
            readers.emplace_back([&]() {
                while (running) {
                    Page p{};
                    disk.read_page(pageId, p);
                    // Just ensuring no crash/segfault during heavy contention
                }
            });
        }

        writer.join();
        for (auto& t : readers) t.join();

        // If we reached here without a crash or TSAN error, the basic thread-safety is intact.
    }

}  // namespace bermudadb