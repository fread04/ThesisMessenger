#pragma once

#include <string>
#include <map>
#include <fstream>
#include <sstream>

class IniParser {
public:
    bool load(const std::string& filename);

    std::string get(const std::string& section, const std::string& key, const std::string& defaultValue = "") const;

private:
    std::map<std::string, std::map<std::string, std::string>> data;
};

