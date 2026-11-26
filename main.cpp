#include "split_colors.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <opencv2/core/utils/logger.hpp>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <conio.h>
#endif

namespace {
using namespace split;
bool inside(const fs::path& path,const fs::path& parent) {
    auto p=path.begin();
    for (auto b=parent.begin();b!=parent.end();++b,++p) if (p==path.end() || *p!=*b) return false;
    return true;
}

// Reserve our own temporary directory and never overwrite an existing result.
std::pair<fs::path,fs::path> reserveOutput(const fs::path& output,const fs::path& groupName) {
    for (int index=1;index<100000;++index) {
        fs::path name=groupName;
        if (index>1) name += "_"+std::to_string(index);
        fs::path target=output/name, temporary=target;
        temporary += ".split_tmp";
        if (fs::exists(target) || fs::exists(temporary)) continue;
        if (fs::create_directory(temporary)) return {target,temporary};
    }
    throw std::runtime_error("Cannot reserve an output directory");
}

void waitIfStartedFromExplorer() {
#ifdef _WIN32
    DWORD mode=0;
    if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode)) return;
    HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot==INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W entry{};
    entry.dwSize=sizeof(entry);
    DWORD parentId=0;
    if (Process32FirstW(snapshot,&entry)) {
        do {
            if (entry.th32ProcessID==GetCurrentProcessId()) { parentId=entry.th32ParentProcessID; break; }
        } while (Process32NextW(snapshot,&entry));
    }
    bool fromExplorer=false;
    if (parentId && Process32FirstW(snapshot,&entry)) {
        do {
            if (entry.th32ProcessID==parentId) { fromExplorer=_wcsicmp(entry.szExeFile,L"explorer.exe")==0; break; }
        } while (Process32NextW(snapshot,&entry));
    }
    CloseHandle(snapshot);
    if (fromExplorer) {
        std::cout << "\nPress any key to close..." << std::flush;
        _getwch();
    }
#endif
}
}

int run(int argc,char* argv[]) {
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);
    try {
        if (argc>2) throw std::runtime_error("Usage: SplitColors [path-to-.env]");
        fs::path configPath;
        if (argc==2) configPath=fs::u8path(argv[1]);
        else if (fs::exists(fs::current_path()/".env")) configPath=fs::current_path()/".env";
        else configPath=split::executableDirectory(argv[0])/".env";
        auto config=split::readConfig(configPath);
        // std::cout << "Config: " << split::utf8(fs::absolute(configPath)) << '\n';
        if (!fs::is_directory(config.input)) throw std::runtime_error("SC_INPUT_DIR is not a directory");
        fs::path input=fs::canonical(config.input), output=fs::weakly_canonical(config.output);
        if (inside(output,input)) throw std::runtime_error("SC_OUTPUT_DIR must not be inside SC_INPUT_DIR");
        fs::create_directories(output);
        std::vector<fs::path> groups;
        if (!split::findImages(input).empty()) groups.push_back(input);
        for (const auto& entry:fs::directory_iterator(input))
            if (entry.is_directory() && !split::findImages(entry.path()).empty()) groups.push_back(entry.path());
        std::sort(groups.begin(),groups.end());
        if (groups.empty()) throw std::runtime_error("No input images found");
        int succeeded=0,failed=0;
        for (const auto& group:groups) {
            fs::path temporary;
            try {
                auto images=split::findImages(group);
                fs::path source=split::chooseSource(images,config);
                images.erase(std::remove(images.begin(),images.end(),source),images.end());
                fs::path groupName=group==input ? source.filename() : group.filename();
                auto reserved=reserveOutput(output,groupName);
                fs::path target=reserved.first; temporary=reserved.second;
                int count=split::splitGroup(source,images,temporary,config);
                fs::rename(temporary,target); temporary.clear();
                std::cout << "OK: " << split::utf8(group) << ": " << count << " layers -> " << split::utf8(target) << '\n';
                ++succeeded;
            } catch (const std::exception& e) {
                if (!temporary.empty()) { std::error_code ignored; fs::remove_all(temporary,ignored); }
                std::cerr << "FAILED: " << split::utf8(group) << ": " << e.what() << '\n';
                ++failed;
            }
        }
        std::cout << "Groups: " << groups.size() << ", succeeded: " << succeeded << ", failed: " << failed << '\n';
        return failed ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}

int main(int argc,char* argv[]) {
    int result=run(argc,argv);
    waitIfStartedFromExplorer();
    return result;
}
