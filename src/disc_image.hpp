#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace ps1 {
struct DiscImage {
 enum class Mode {Mode1,Mode2,Audio};
 struct File {std::filesystem::path path;std::ifstream stream;uint64_t bytes=0;};
 struct Track {
  unsigned number=0,file=0,stride=2352;Mode mode=Mode::Mode2;
  uint32_t index0=0,index1=0,pregap=0,start=0,end=0,gap_start=0;
  bool has0=false,has1=false;
 };
 std::vector<File> files;
 std::vector<Track> tracks;
 std::filesystem::path path;
 uint32_t sectors=0;
 void open(const std::filesystem::path& filename);
 const Track& track(uint32_t lba)const;
 std::array<uint8_t,2352> raw(uint32_t lba);
 std::vector<uint8_t> user(uint32_t lba);
 std::string description()const;
};
}
