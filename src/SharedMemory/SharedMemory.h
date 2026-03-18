#pragma once
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
namespace backtesting_engine {
class SharedMemory {
public:
  SharedMemory(const std::string &name, size_t size, bool create = true)
      : name_(name), size_(size), created_(create) {

    int flags = O_RDWR; // read and write flag
    if (create)
      flags |= O_CREAT; // create flag

    while (true) {
      fd_ = shm_open(name_.c_str(), flags, 0666);
      if (fd_ != -1) {
        if (!create) {
          // Check if producer thread has called ftruncate to prevent race
          // condition of consumer calling mmap before producer calls ftruncate
          struct stat statbuf;
          if (fstat(fd_, &statbuf) == 0 && statbuf.st_size >= size_) {
            break;
          }
          close(fd_);
          usleep(100);
          continue;
        }
        break;
      }

      if (create) {
        throw std::runtime_error("Failed to open shared memory: " + name_);
      }
    }
    std::cout << "SHM opened: " << name_ << std::endl;

    if (create) {
      if (ftruncate(fd_, size_) == -1) {
        unlink();
        throw std::runtime_error("Failed to set size of shared memory");
      }
    }
    ptr_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
    if (ptr_ == MAP_FAILED) {
      unlink();
      throw std::runtime_error("Failed to mmap shared memory (" +
                               std::to_string(errno) +
                               "): " + std::string(std::strerror(errno)));
    }
  }

  // CAUTION: DESTRUCTOR DOES NOT UNLINK THE SHARED MEMORY
  // THIS IS INTENTIONAL, SO THAT THE SHARED MEMORY PERSISTS AFTER THE PROCESS
  // EXITS FOR DEBUGGING PURPOSES ON LINUX.
  // CALL UNLINK MANUALLY WHEN YOU WANT TO DELETE THE SHARED MEMORY.
  ~SharedMemory() {
    if (ptr_ != MAP_FAILED) {
      munmap(ptr_, size_);
    }
    if (fd_ != -1) {
      close(fd_);
    }
  }

  void *get_ptr() const { return ptr_; }

  void unlink() { shm_unlink(name_.c_str()); }

private:
  std::string name_;
  size_t size_;
  int fd_;
  void *ptr_;
  bool created_;
};
} // namespace backtesting_engine
