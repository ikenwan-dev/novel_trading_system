#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include <string>
#include <vector>

#include <algorithm>
#include <filesystem>
#include <iostream>

std::vector<std::string> get_dbn_files(const std::string &directory) {
  std::vector<std::string> files;
  const std::string suffix = ".mbo.dbn.zst";
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      std::string path = entry.path().string();
      if (path.size() >= suffix.size() &&
          path.compare(path.size() - suffix.size(), suffix.size(), suffix) ==
              0) {
        files.push_back(path);
      }
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

int main() {
  std::string directory =
      std::string(PROJECT_ROOT) + "/src/test_data/XNAS-20260120-7W93CD9NGT/";
  std::vector<std::string> filepaths = get_dbn_files(directory);
  for (const auto &filepath : filepaths) {
    std::cout << filepath << '\n';
  }

  DataBentoProducer::Interactor writer("test_shm");
  DataBentoProducer dataBentoProducer(filepaths, writer);
  dataBentoProducer.produce();
  writer.unlink();
  return 0;
}