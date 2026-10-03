#include "bermudadb/storage/DiskManager.hpp"
#include "bermudadb/common/Logger.hpp"

namespace bermudadb {

    DiskManager::DiskManager(const std::filesystem::path &path, Logger &logger) : logger_(logger){
        logger_.debug("DiskManager: Opening file at " + path.string());
        file_.open(path, std::ios::binary | std::ios::in | std::ios::out);

        if (!file_.is_open()) {
            logger_.warning("DiskManager: File not open initially, attempting to create and open.");
            std::ofstream createFile(path, std::ios::binary);
            createFile.close();

            file_.open(path, std::ios::binary | std::ios::in | std::ios::out);
            if (!file_.is_open()) {
                logger_.error("DiskManager: Failed to open file after creation attempt: " + path.string());
            }
        }

           // Set up the first bitmap on page 0
            if (file_.is_open()) {
                file_.seekg(0, std::ios::end);
                std::streamsize size = file_.tellg();
                if (size == 0) {
                    writePageUnlocked(0, blankPage);
                }
            } else {
                logger_.error("DiskManager: Failed to set up initial bitmap - file is not open.");
            }
    }

    PageId DiskManager::allocatePage() {
        std::lock_guard lock(mutex_);
        logger_.debug("DiskManager::allocatePage - Starting allocation");

        for (PageId region = 0; ;region++) {
            const PageId bitmapPageId = region * PAGES_PER_REGION;

            Page bitmapPage{};
            readPageUnlocked(bitmapPageId, bitmapPage);
            if (file_.eof()) {
                file_.clear();
                writePageUnlocked(bitmapPageId, blankPage);
            }

            // Try to locate a empty page from the bitmap
            for (std::size_t i = 0; i < PAGE_SIZE; ++i) {
                for (int bit = 0; bit < 8; ++bit) {
                    if (!(static_cast<uint8_t>(bitmapPage.data[i]) & (1 << bit))) {
                        bitmapPage.data[i] |= static_cast<std::byte>(1 << bit);
                        writePageUnlocked(bitmapPageId, bitmapPage);
                        return bitmapPageId + i * 8 + bit + 1;
                    }
                }
            }
        }
    }

    bool DiskManager::freePage(const PageId pageId) {
        // Can't free a bitmap
        if (pageId % PAGES_PER_REGION == 0) {
            logger_.error("DiskManager::freePage - Attempting to free a bitmap page: " + std::to_string(pageId));
            return false;
        }

        const PageId bitmapPageId = findBitmapForPage(pageId);

        Page bitmapPage{};
        readPageUnlocked(bitmapPageId, bitmapPage);

        if (!file_.is_open()) {
            logger_.error("DiskManager::freePage - File is not open. is_open(): " + std::to_string(!file_.is_open()));
            return false;
        }

        const auto pageOffset = pageId % PAGES_PER_REGION - 1;
        const auto byteIndex = pageOffset / 8;
        const auto bitIndex = pageOffset % 8;

        bitmapPage.data[byteIndex] &= ~(std::byte{1} << bitIndex);

        writePageUnlocked(bitmapPageId, bitmapPage);

        bool success = static_cast<bool>(file_);
        if (!success) {
            logger_.error("DiskManager::freePage - Write failed. State: " + std::to_string(static_cast<int>(file_.rdstate())));
        }
        return success;
    }

    void DiskManager::readPage(const PageId pageId, Page &page) {
        std::lock_guard lock(mutex_);
        logger_.debug("DiskManager::readPage - PageId: " + std::to_string(pageId) + " | File Open: " + std::to_string(file_.is_open()));
        readPageUnlocked(pageId, page);
    }

    void DiskManager::writePage(const PageId pageId, const Page &page) {
        std::lock_guard lock(mutex_);
        logger_.debug("DiskManager::writePage - PageId: " + std::to_string(pageId) + " | File Open: " + std::to_string(file_.is_open()));
        writePageUnlocked(pageId, page);
    }

    PageId DiskManager::findBitmapForPage(const PageId pageId) {
        return (pageId / PAGES_PER_REGION) * PAGES_PER_REGION;
    }

    void DiskManager::readPageUnlocked(const PageId pageId, Page &page) {
        const auto offset = pageId * PAGE_SIZE;
        file_.seekg(offset);
        file_.read(reinterpret_cast<char*>(page.data.data()), PAGE_SIZE);
        if (file_.fail()) {
            logger_.error("DiskManager::readPageUnlocked - Seek/Read failed at offset " + std::to_string(offset) + " | State: " + std::to_string(static_cast<int>(file_.rdstate())));
            file_.clear();
        }
    }

    void DiskManager::writePageUnlocked(const PageId pageId, const Page &page) {
        const auto offset = pageId * PAGE_SIZE;
        file_.seekp(offset);
        file_.write(reinterpret_cast<const char*>(page.data.data()), PAGE_SIZE);
        if (file_.fail()) {
            logger_.error("DiskManager::writePageUnlocked - Seek/Write failed at offset " + std::to_string(offset) + " | State: " + std::to_string(static_cast<int>(file_.rdstate())));
            file_.clear();
        }
    }
}