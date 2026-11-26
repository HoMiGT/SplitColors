#include "split_colors.hpp"
#include <fstream>
#include <map>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace split {
std::string utf8(const fs::path& path) {
    auto text=path.u8string();
    return std::string(text.begin(),text.end());
}

fs::path executableDirectory(const char* argv0) {
#ifdef _WIN32
    (void)argv0;
    std::vector<wchar_t> buffer(512);
    for (;;) {
        DWORD length=GetModuleFileNameW(nullptr,buffer.data(),static_cast<DWORD>(buffer.size()));
        if (!length) throw std::runtime_error("Cannot locate executable");
        if (length<buffer.size()) return fs::path(std::wstring(buffer.data(),length)).parent_path();
        if (buffer.size()>=32768) throw std::runtime_error("Executable path is too long");
        buffer.resize(buffer.size()*2);
    }
#elif defined(__linux__)
    (void)argv0;
    return fs::canonical("/proc/self/exe").parent_path();
#else
    return fs::absolute(fs::path(argv0)).parent_path();
#endif
}

static std::string trim(const std::string& s) {
    auto first=s.find_first_not_of(" \t\r\n");
    if (first==std::string::npos) return {};
    return s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}

static fs::path fromUtf8(const std::string& s) {
    return fs::path(std::u8string(s.begin(),s.end()));
}

Config readConfig(const fs::path& file) {
    std::ifstream in(file,std::ios::binary);
    if (!in) throw std::runtime_error("Cannot read configuration: "+utf8(file));
    std::map<std::string,std::string> values;
    std::string line;
    int lineNumber=0;
    while (std::getline(in,line)) {
        ++lineNumber;
        if (lineNumber==1 && line.compare(0,3,"\xEF\xBB\xBF")==0) line.erase(0,3);
        line=trim(line);
        if (line.empty() || line[0]=='#') continue;
        auto equals=line.find('=');
        if (equals==std::string::npos) throw std::runtime_error("Invalid .env line "+std::to_string(lineNumber));
        auto key=trim(line.substr(0,equals)), value=trim(line.substr(equals+1));
        if (key.empty()) throw std::runtime_error("Empty key in .env line "+std::to_string(lineNumber));
        if (!value.empty() && (value.front()=='\'' || value.front()=='"')) {
            if (value.size()<2 || value.front()!=value.back()) throw std::runtime_error("Unclosed quote in .env line "+std::to_string(lineNumber));
            value=value.substr(1,value.size()-2);
        }
        if (!values.emplace(key,value).second) throw std::runtime_error("Duplicate .env key: "+key);
    }
    if (in.bad()) throw std::runtime_error("Error reading configuration");
    const auto directory=fs::absolute(file).parent_path();
    auto pathValue=[&](const char* key) {
        const auto& text=values[key];
        if (text.empty()) throw std::runtime_error(std::string("Missing configuration: ")+key);
        fs::path result=fromUtf8(text);
        return result.is_absolute() ? result : directory/result;
    };
    Config config;
    config.input=pathValue("SC_INPUT_DIR");
    config.output=pathValue("SC_OUTPUT_DIR");
    config.sourceName=fromUtf8(values["SC_SOURCE_IMAGE"]);
    if (!config.sourceName.empty() && (config.sourceName.has_parent_path() || config.sourceName.is_absolute()))
        throw std::runtime_error("SC_SOURCE_IMAGE must be a filename, not a path");
    auto foreground=values["MASK_FOREGROUND"];
    if (!foreground.empty() && foreground!="black" && foreground!="white") throw std::runtime_error("MASK_FOREGROUND must be black or white");
    config.blackForeground=foreground!="white";
    auto edges=values["MASK_EDGES"];
    if (!edges.empty() && edges!="soft" && edges!="binary") throw std::runtime_error("MASK_EDGES must be soft or binary");
    config.softEdges=edges!="binary";
    const auto threshold=values["MASK_THRESHOLD"];
    if (!threshold.empty()) {
        try {
            size_t used=0; int value=std::stoi(threshold,&used);
            if (used!=threshold.size() || value<1 || value>255) throw std::invalid_argument("threshold");
            config.threshold=value;
        } catch (const std::exception&) { throw std::runtime_error("MASK_THRESHOLD must be an integer between 1 and 255"); }
    }
    return config;
}
}
