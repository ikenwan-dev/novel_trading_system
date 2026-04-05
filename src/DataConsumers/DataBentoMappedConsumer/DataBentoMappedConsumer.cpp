#include "DataConsumers/DataBentoMappedConsumer/DataBentoMappedConsumer.h"
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace backtesting_engine::mbo {

DataBentoMappedConsumer::DataBentoMappedConsumer(
    const std::vector<std::string> &filepaths)
    : filepaths_(filepaths), current_file_index_(0), fd_(-1), mapped_size_(0),
      mmap_base_(nullptr), current_ptr_(nullptr) {
  open_next_file();
}

DataBentoMappedConsumer::~DataBentoMappedConsumer() { unmap_current(); }

void DataBentoMappedConsumer::unmap_current() {
  if (mmap_base_ && mmap_base_ != MAP_FAILED) {
    munmap(const_cast<char *>(mmap_base_), mapped_size_);
    mmap_base_ = nullptr;
  }
  if (fd_ != -1) {
    close(fd_);
    fd_ = -1;
  }
}

void DataBentoMappedConsumer::open_next_file() {
  unmap_current();

  if (current_file_index_ >= filepaths_.size()) {
    return; // EOF
  }

  const std::string &path = filepaths_[current_file_index_++];
  fd_ = open(path.c_str(), O_RDONLY);
  if (fd_ == -1) {
    std::cerr << "Failed to open " << path << std::endl;
    return;
  }

  struct stat sb;
  if (fstat(fd_, &sb) == -1) {
    std::cerr << "Failed to stat " << path << std::endl;
    close(fd_);
    fd_ = -1;
    return;
  }
  mapped_size_ = sb.st_size;

  mmap_base_ = static_cast<const char *>(
      mmap(nullptr, mapped_size_, PROT_READ, MAP_PRIVATE, fd_, 0));
  if (mmap_base_ == MAP_FAILED) {
    std::cerr << "Failed to mmap " << path << std::endl;
    close(fd_);
    fd_ = -1;
    return;
  }

  // Advise the kernel that we will read this file sequentially and we need it
  // now. This triggers background page-faulting (read-ahead).
  posix_madvise(const_cast<char *>(mmap_base_), mapped_size_,
                POSIX_MADV_SEQUENTIAL);
  posix_madvise(const_cast<char *>(mmap_base_), mapped_size_,
                POSIX_MADV_WILLNEED);

  // DBN header: 'DBN\0' + 4 bytes length (little endian)
  if (mapped_size_ < 8) {
    current_ptr_ = mmap_base_ + mapped_size_;
    return;
  }

  uint32_t header_length = *reinterpret_cast<const uint32_t *>(mmap_base_ + 4);
  current_ptr_ = mmap_base_ + 8 + header_length;
}

bool DataBentoMappedConsumer::try_poll(databento::MboMsg &out_msg) {
  while (true) {
    if (!mmap_base_) {
      // Simulate EOF signal by returning an empty message
      out_msg = databento::MboMsg{};
      return true;
    }

    if (current_ptr_ >= mmap_base_ + mapped_size_) {
      open_next_file();
      continue;
    }

    const databento::RecordHeader *header =
        reinterpret_cast<const databento::RecordHeader *>(current_ptr_);
    const databento::MboMsg *mbo =
        reinterpret_cast<const databento::MboMsg *>(current_ptr_);

    out_msg = *mbo; // Copy the record exactly

    // header->length is in 32-bit words
    current_ptr_ += (header->length * 4);

    // Ensure we only process actual MBO records (skip SymbolMappingMsg etc. if
    // any)
    if (header->rtype == databento::RType::Mbo) {
      return true;
    }
  }
}

} // namespace backtesting_engine::mbo
