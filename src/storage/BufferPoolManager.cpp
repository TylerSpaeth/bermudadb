#include "bermudadb/storage/BufferPoolManager.hpp"

#include <iostream>

namespace bermudadb {

    BufferPoolManager::BufferPoolManager(std::size_t poolSize, DiskManager &diskManager) : poolSize_(poolSize), clockPointer_(0), diskManager_(diskManager), frames_(poolSize) {}

    Page* BufferPoolManager::fetchPage(PageId pageId) {

        std::lock_guard lock(mutex_);

        if (const auto frameId = pageTable_.find(pageId); frameId != pageTable_.end()) {
            auto& frame = frames_.at(frameId->second);
            frame.pinCount_++;
            frame.referenceBit_ = true;
            return &frame.page_;
        }

        return placePageInBufferPool(pageId);
    }

    bool BufferPoolManager::unpinPage(const PageId pageId, const bool isDirty) {

        std::lock_guard lock(mutex_);

        const auto frameId = pageTable_.find(pageId);
        if (frameId == pageTable_.end()) {
            return false;
        }

        auto& frame = frames_.at(frameId->second);

        if (frame.pinCount_ == 0) {
            return false;
        }

        frame.dirty_ = frame.dirty_ || isDirty;
        frame.pinCount_--;
        return true;
    }

    bool BufferPoolManager::flushPage(PageId pageId) {

        std::lock_guard lock(mutex_);

        const auto frameId = pageTable_.find(pageId);
        if (frameId == pageTable_.end()) {
            return false;
        }

        auto& frame = frames_.at(frameId->second);
        diskManager_.writePage(pageId, frame.page_);
        frame.dirty_ = false;
        return true;
    }

    Page* BufferPoolManager::createPage(PageId& pageId) {
        std::lock_guard lock(mutex_);
        pageId = diskManager_.allocatePage();
        try {
            return placePageInBufferPool(pageId);
        } catch (...) {
            diskManager_.freePage(pageId);
            throw;
        }
    }

    bool BufferPoolManager::deletePage(PageId pageId) {
        std::lock_guard lock(mutex_);

        const auto frameId = pageTable_.find(pageId);

        if (frameId != pageTable_.end()) {
            auto& frame = frames_.at(frameId->second);

            if (frame.pinCount_ > 0) {
                return false;
            }
        }

        if (!diskManager_.freePage(pageId)) {
            return false;
        }

        if (frameId != pageTable_.end()) {
            auto& frame = frames_.at(frameId->second);

            pageTable_.erase(frameId);

            frame.pageId_ = 0;
            frame.pinCount_ = 0;
            frame.dirty_ = false;
            frame.referenceBit_ = false;
            frame.occupied_ = false;
        }

        return true;
    }

    Page* BufferPoolManager::placePageInBufferPool(PageId pageId) {
        auto targetToEvict = findTargetForEviction();
        auto& frame = frames_.at(targetToEvict);

        if (frame.occupied_) {
            pageTable_.erase(frame.pageId_);

            if (frame.dirty_) {
                diskManager_.writePage(frame.pageId_, frame.page_);
                frame.dirty_ = false;
            }
        }

        frame.pinCount_++;
        frame.referenceBit_ = true;
        frame.pageId_ = pageId;
        frame.occupied_ = true;
        pageTable_.insert({pageId, targetToEvict});
        diskManager_.readPage(pageId, frame.page_);
        return &frame.page_;
    }

    FrameId BufferPoolManager::findTargetForEviction() {

        for (int i = 0; i < frames_.size() * 2; i++) {
            auto& currentFrame = frames_.at(clockPointer_);
            const auto currentFrameId = clockPointer_;
            clockPointer_ = (clockPointer_ + 1) % poolSize_;

            if (!currentFrame.occupied_) {
                return currentFrameId;
            }

            if (currentFrame.pinCount_ > 0) {
                continue;
            }
            if (currentFrame.referenceBit_) {
                currentFrame.referenceBit_ = false;
                continue;
            }

            return currentFrameId;
        }

        throw std::runtime_error("No space left in buffer pool");
    }
}
