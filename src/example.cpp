#include <databento/historical.hpp>
#include <iostream>

int main() {
  databento::DbnFileStore dbnFileStore = databento::DbnFileStore(
      std::string(PROJECT_ROOT) + "/src/test_data/XNAS-20260120-7W93CD9NGT/"
                                  "xnas-itch-20251223.mbo.dbn.zst");
  std::cout << dbnFileStore.GetMetadata() << '\n';
  //   long long total = 0;
  //   while (const databento::Record *record = dbnFileStore.NextRecord()) {
  //     total++;
  //   }
  //   std::cout << total << '\n';
  for (int i = 0; i < 10; i++) {
    // std::cout << i << "\n";
    const databento::Record *record = dbnFileStore.NextRecord();
    const auto &trade_msg = record->Get<databento::MboMsg>();
    const auto flags = trade_msg.flags;
    // std::cout << *record << "\n";
    // std::cout << (record->Size()) << "\n";
    std::cout << flags << "\n";
    std::cout << trade_msg << "\n \n";
  }
  return 0;
}