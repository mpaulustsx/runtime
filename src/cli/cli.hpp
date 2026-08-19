#pragma once
#include "../runtime/logging.h"
#include "../runtime/runtime.h"

#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <utility>

namespace TCLAP
{
    class CmdLine;
}

class cli
{
    StdOutLogger m_logger;
    sqf::runtime::runtime m_runtime;
    bool m_cli_file;
    bool m_parse_only;
    bool m_good;
    bool m_automated;
    std::unordered_map<std::string, std::vector<std::function<std::pair<std::filesystem::path, std::string>()>>> m_files;
    // physical -> virtual, as given to -v. Kept around so a top-level
    // -i/--input-sqf file can be told which virtual directory it lives in,
    // the same way a file reached through #include already is - without it,
    // the entry file's own "..\..." includes have nothing to resolve
    // against and always fail.
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> m_virtual_mappings;
    std::string virtual_path_for(const std::filesystem::path& physical) const;

    void handle_files();
    void mount_filesystem(const std::vector<std::string>& mappings);
    int cli_from_file(const char* arg0, std::filesystem::path path);
public:
    cli();
    int run(size_t argc, const char** argv);

    bool verbose() const { return m_logger.isEnabled(loglevel::verbose); }
};