#ifndef BERMUDADB_DISKMANAGER_H
#define BERMUDADB_DISKMANAGER_H
#include <filesystem>
#include <fstream>
#include <mutex>

#include "Page.hpp"
#include "bermudadb/common/Logger.hpp"

namespace bermudadb {

    constexpr std::size_t BIT_PER_BYTE = 8;
    constexpr std::size_t PAGES_PER_BITMAP = PAGE_SIZE * BIT_PER_BYTE;
    constexpr std::size_t PAGES_PER_REGION = PAGES_PER_BITMAP + 1;
    constexpr Page blankPage{};

    /**
     * The DiskManager class provides an interface for performing low-level disk I/O
     * operations as well as utilities for managing disk space and ensuring data integrity.
     * It facilitates efficient and reliable data storage by abstracting the underlying
     * hardware-specific disk operations.
     */
    class DiskManager {
        public:
        explicit DiskManager(const std::filesystem::path& path, Logger& logger);

        /**
         * Allocates a new page in the storage system and returns its unique identifier.
         * The allocation process identifies an unused page using a bitmap-based mechanism,
         * marks the page as allocated, and ensures persistence through disk writes.
         *
         * Thread safety is ensured by synchronizing access to the disk file during the operation.
         *
         * @return The unique identifier (PageId) of the newly allocated page.
         */
        PageId allocatePage();

        /**
         * Frees a previously allocated page in the storage system, making it available
         * for future allocations. The operation updates the corresponding bitmap to mark
         * the page as free and persists the changes to disk.
         *
         * The method ensures that any attempt to free a reserved bitmap page is denied and
         * returns false in such cases. Proper synchronization is maintained to ensure
         * thread safety during the operation.
         *
         * @param pageId The unique identifier of the page to be freed.
         * @return A boolean value indicating the success of the operation. Returns `true`
         *         if the page was successfully freed, `false` otherwise.
         */
        bool freePage(PageId pageId);

        /**
         * Reads the contents of a specific page from storage into memory.
         * This method retrieves the data associated with the given page identifier
         * and loads it into the provided buffer for further operations.
         *
         * @param pageId The unique identifier of the page to be read.
         * @param page The memory location where the page data will be stored.
         */
        void readPage(PageId pageId, Page& page);

        /**
         * Writes a page of data to the specified location in storage.
         * This method ensures that the page is properly persisted and
         * integrated with the storage medium.
         *
         * @param pageId The unique identifier of the page to be written.
         * @param page A buffer containing the data to be written to the page.
         */
        void writePage(PageId pageId, const Page& page);

        private:
        Logger& logger_;

        std::fstream file_;
        std::mutex mutex_;

        static PageId findBitmapForPage(PageId pageId);

        void readPageUnlocked(PageId pageId, Page& page);
        void writePageUnlocked(PageId pageId, const Page& page);
    };
}

#endif //BERMUDADB_DISKMANAGER_H