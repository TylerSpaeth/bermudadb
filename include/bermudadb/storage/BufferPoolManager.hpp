#ifndef BERMUDADB_BUFFERPOOLMANAGER_H
#define BERMUDADB_BUFFERPOOLMANAGER_H
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "DiskManager.hpp"
#include "Frame.hpp"
#include "Page.hpp"

namespace bermudadb {

    /**
     * Initializes a BufferPoolManager with a given pool size and reference to a DiskManager.
     *
     * @param poolSize The number of frames available in the buffer pool.
     * @param diskManager Reference to the DiskManager for performing disk I/O operations.
     */
    class BufferPoolManager {
    public:
        BufferPoolManager(std::size_t poolSize, DiskManager& diskManager);

        /**
         * Retrieves a page from the buffer pool. If the page is not in memory,
         * it is loaded from disk into a free frame.
         *
         * @param pageId The unique identifier for the page to fetch.
         * @return A pointer to the page in the buffer pool.
         */
        Page* fetchPage(PageId pageId);


        /**
         * Decreases the pin count of a page and optionally marks it dirty.
         * If the page is not found in the buffer (not loaded), the function returns false.
         *
         * @param pageId The unique identifier for the page to be unpinned.
         * @param isDirty Indicates whether the page has been modified.
         * @return True if the page was successfully unpinned, false otherwise.
         */
        bool unpinPage(PageId pageId, bool isDirty);


        /**
         * Flushes the specified page to disk if it is currently loaded in the buffer.
         *
         * @param pageId The unique identifier for the page to be flushed.
         * @return True if the page was successfully flushed to disk, false otherwise.
         */
        bool flushPage(PageId pageId);

        /**
         * Allocates a new page in the database and places it in the buffer pool.
         *
         * @param pageId Reference to a PageId that will be populated with the new ID.
         * @return A pointer to the newly allocated page in the buffer pool.
         */
        Page* createPage(PageId& pageId);

        /**
         * Deletes a page from the database and removes it from the buffer pool.
         * The page must not be pinned by any active threads.
         *
         * @param pageId The unique identifier for the page to be deleted.
         * @return True if the page was successfully deleted, false otherwise.
         */
        bool deletePage(PageId pageId);


    private:
        std::size_t poolSize_;
        std::vector<Frame> frames_;
        std::unordered_map<PageId, FrameId> pageTable_;
        std::size_t clockPointer_;

        DiskManager& diskManager_;

        std::mutex mutex_;

        Page* placePageInBufferPool(PageId pageId);

        /**
         * Implementation of the clock algorithm to find the target that needs to be evicted next.
         * @return FrameId to be evicted
         */
        FrameId findTargetForEviction();
    };

}

#endif // BERMUDADB_BUFFERPOOLMANAGER_H