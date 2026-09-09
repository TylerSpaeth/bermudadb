#ifndef BERMUDADB_DISKMANAGER_H
#define BERMUDADB_DISKMANAGER_H
#include <filesystem>
#include <fstream>
#include <mutex>

#include "page.hpp"

namespace bermudadb {
    /**
     * The DiskManager class provides an interface for performing low-level disk I/O
     * operations as well as utilities for managing disk space and ensuring data integrity.
     * It facilitates efficient and reliable data storage by abstracting the underlying
     * hardware-specific disk operations.
     */
    class DiskManager {
        public:
        explicit DiskManager(const std::filesystem::path& path);

        void read_page(std::uint64_t pageId, Page& page);
        void write_page(std::uint64_t pageId, const Page& page);

        private:
        std::fstream m_File;
        std::mutex m_Mutex;
    };
}

#endif //BERMUDADB_DISKMANAGER_H