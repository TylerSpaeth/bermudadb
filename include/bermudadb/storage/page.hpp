#ifndef BERMUDADB_PAGE_H
#define BERMUDADB_PAGE_H
#include <array>

namespace bermudadb {

    constexpr std::size_t PAGE_SIZE = 4096;

    struct Page {
        std::array<std::byte, PAGE_SIZE> data;
    };
}

#endif //BERMUDADB_PAGE_H