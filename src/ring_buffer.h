#pragma once

#include <cassert>
#include <vector>
#include <memory>
#include <atomic>
#include <span>

#include "util.h"


// Implementation of a nonblocking ring buffer.
// It uses strong memory order assumption, so probably
// only works correctly on X86 CPU.
template <typename T>
class RingBuffer {
 public:
  explicit RingBuffer(std::size_t size)
      : size_(size), data_(size) {
    read_position_.store(0);
    write_position_.store(0);
  }

  // Write N bytes from current position. 
  // N must be <= size.
  // src must contain atleast N elements.
  void Write(const std::vector<T>& src, std::size_t n) {
    ASSERT(n <= size_);
    auto head = write_position_.load();
    auto position = head % size_;

    if (n > size_ - position) {
      std::size_t part_size = size_ - position;
      std::copy(src.begin(), src.begin() + part_size, data_.begin() + position);
      std::copy(src.begin() + part_size, src.begin() + n, data_.begin());
    } else {
      std::copy(src.begin(), src.begin() + n, data_.begin() + position);
    }

    write_position_.store(head + n);
  }

  // Read N elements into dst. Assume dst already resized to atleast N.
  void Read(std::vector<T>& dst, std::size_t n) {
    auto head = read_position_.load();
    auto position = head % size_;
    if (n > size_ - position) {
      std::copy(data_.begin() + position, data_.end(), dst.begin());
      std::size_t num_copied = size_ - position;
      std::size_t remaining = n - num_copied;
      std::copy(data_.begin(), data_.begin() + remaining, dst.begin() + num_copied);
    } else {
      std::copy(data_.begin() + position, data_.begin() + position + n, dst.begin());
    }

    read_position_.store(head + n);
  }

  void Read(T* dst, std::size_t n) {
    std::span<T> sp(dst, n);
    auto head = read_position_.load();
    auto position = head % size_;
    if (n > size_ - position) {
      std::copy(data_.begin() + position, data_.end(), sp.begin());
      std::size_t num_copied = size_ - position;
      std::size_t remaining = n - num_copied;
      std::copy(data_.begin(), data_.begin() + remaining, sp.begin() + num_copied);
    } else {
      std::copy(data_.begin() + position, data_.begin() + position + n, sp.begin());
    }

    read_position_.store(head + n);
  }

  // This many elements can be read when the function is called.
  // If you read more, result could be undefined.
  std::size_t ReadyToRead() const {
    return write_position_.load() - read_position_.load();
  }

  // This much data can be written when the function is called.
  // If you write more, you may run into conflict with the reader.
  std::size_t ReadyToWrite() const {
    std::size_t delta = write_position_.load() - read_position_.load();
    assert(delta <= size_);
    return size_ - delta;
  }

  // Size of the buffer. It can't be changed.
  std::size_t Size() const {
    return size_;
  }

  // Current write head position. It always increases when new data is written.
  std::size_t Position() const {
    return write_position_.load();
  }

 private:
  const std::size_t size_;
  std::vector<T> data_;
  std::atomic<std::size_t> read_position_;
  std::atomic<std::size_t> write_position_;
};
