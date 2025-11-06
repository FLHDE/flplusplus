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
        float pbubblescale;
        float characterdetailscale;
        float asteroiddistscale;
        std::string savefoldername;
        bool saveindirectory;
        std::string screenshotsfoldername;
        bool screenshotsindirectory;
        bool altfullscreenscreenshots;
        bool altwindowedscreenshots;
        bool removestartlocationwarning;
        bool logtoconsole;
        float shippreviewscrollingspeed;
        bool shippreviewscrollinginverse;
        float shippreviewscrollingmindistance;
        float shippreviewscrollingmaxdistance;
        bool alwaysregeneraterestartfile;
        int failedtoinitsavesdirids;
        bool touchpadsupport;
        std::vector<std::string> fontfiles{};
        std::vector<std::string> directips{};
    };
    ConfigData& get_config();
}
