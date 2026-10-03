#ifndef BERMUDADB_FRAME_HPP
#define BERMUDADB_FRAME_HPP

#include "Page.hpp"

namespace bermudadb {

    using FrameId = uint64_t;

    struct Frame {
        Page page_{};
        PageId pageId_{};
        std::size_t pinCount_ = 0;
        bool dirty_ = false;
        bool referenceBit_ = false;
        bool occupied_ = false;
    };
}

#endif //BERMUDADB_FRAME_HPP