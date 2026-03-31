#include "DataProducers/DataBentoProducer/DataBentoProducer.h"
#include <string>
#include <vector>

#include <algorithm>
#include <filesystem>

using namespace backtesting_engine;
using namespace backtesting_engine::mbo;

std::vector<std::string> get_dbn_files(const std::string &directory) {
  std::vector<std::string> files;
  const std::string suffix = ".mbo.dbn";
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

int main(int argc, char* argv[]) {
  std::vector<std::string> filepaths;

  if (argc > 1) {
    filepaths.push_back(argv[1]);
  } else {
    std::string directory =
        std::string(PROJECT_ROOT) + "/src/test_data/XNAS-20260120-7W93CD9NGT/";
    filepaths = get_dbn_files(directory);
  }

  DataBentoProducer::Interactor writer("test_shm");
  DataBentoProducer dataBentoProducer(filepaths, writer);
  dataBentoProducer.produce();
  // writer.unlink();
  return 0;
}