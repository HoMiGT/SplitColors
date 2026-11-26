#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

namespace split {
namespace fs = std::filesystem;

struct Config {
    fs::path input;
    fs::path output;
    fs::path sourceName;
    bool blackForeground = true;
    bool softEdges = true;
    int threshold = 128;
};

Config readConfig(const fs::path& file);
fs::path executableDirectory(const char* argv0);
bool isImage(const fs::path& path);
cv::Mat readImage(const fs::path& path);
void writePng(const fs::path& path, const cv::Mat& image);
std::vector<fs::path> findImages(const fs::path& directory);
fs::path chooseSource(const std::vector<fs::path>& images, const Config& config);
int splitGroup(const fs::path& source, const std::vector<fs::path>& masks,
               const fs::path& output, const Config& config);
std::string utf8(const fs::path& path);
}
