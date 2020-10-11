#pragma once

#include <cassert>
#include <vector>
#include <memory>
#include <atomic>

template <typename T>
class RingBuffer {
 public:
  explicit RingBuffer(std::size_t size)
    : size_(size), data_(size) {
    read_position_.store(0);
    write_position_.store(0);
  }

  // N must be <= size
  void Write(const std::vector<T>& src, std::size_t n) {
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

  // Assume already resized to atleast N.
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

  std::size_t ReadyToRead() const {
    return write_position_.load() - read_position_.load();
  }

  std::size_t ReadyToWrite() const {
    std::size_t delta = write_position_.load() - read_position_.load();
    assert(delta <= size_);
    return size_ - delta;
  }

  std::size_t Size() const {
    return size_;
  }

  std::size_t Position() const {
    return write_position_.load();
  }

 private:
  const std::size_t size_;
  std::vector<T> data_;

  std::atomic<std::size_t> read_position_;
  std::atomic<std::size_t> write_position_;
};
