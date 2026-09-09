#include "bermudadb/storage/DiskManager.hpp"

namespace bermudadb {

    DiskManager::DiskManager(const std::filesystem::path &path) {
        m_File.open(path, std::ios::binary | std::ios::in | std::ios::out);

        if (!m_File.is_open()) {
            std::ofstream createFile(path, std::ios::binary);
            createFile.close();

            m_File.open(path, std::ios::binary | std::ios::in | std::ios::out);
        }
    }

    void DiskManager::read_page(std::uint64_t pageId, Page &page) {
        auto offset = pageId * PAGE_SIZE;
        std::lock_guard lock(m_Mutex);
        m_File.seekg(offset);
        m_File.read(reinterpret_cast<char*>(page.data.data()), PAGE_SIZE);
    }

    void DiskManager::write_page(std::uint64_t pageId, const Page &page) {
        auto offset = pageId * PAGE_SIZE;
        std::lock_guard lock(m_Mutex);
        m_File.seekp(offset);
        m_File.write(reinterpret_cast<const char*>(page.data.data()), PAGE_SIZE);
    }
}
