#pragma once

#include <iostream>
#include <vector>

namespace config {
    void init_defaults();
    void init_from_file(const char *filename);
    void read_font_files(const char *filename);
    class ConfigData
    {
    public:
        float lodscale;
        std::string savefoldername;
        bool saveindirectory;
        bool removestartlocationwarning;
        bool logtoconsole;
        std::vector<std::string> fontfiles{};
    };
    ConfigData& get_config();
}
