#include "Config.h"
#include "Logger.h"

bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_WARNING("Config file not found: " + filename);
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }

        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            values[key] = value;
        }
    }

    LOG_INFO("Config loaded: " + filename);
    return true;
}

bool Config::save(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Cannot save config file: " + filename);
        return false;
    }

    for (const auto& pair : values) {
        file << pair.first << " = " << pair.second << std::endl;
    }

    LOG_INFO("Config saved: " + filename);
    return true;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = values.find(key);
    return (it != values.end()) ? it->second : defaultValue;
}

int Config::getInt(const std::string& key, int defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            LOG_WARNING("Invalid int value for key: " + key);
            return defaultValue;
        }
    }
    return defaultValue;
}

float Config::getFloat(const std::string& key, float defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        try {
            return std::stof(it->second);
        } catch (...) {
            LOG_WARNING("Invalid float value for key: " + key);
            return defaultValue;
        }
    }
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) const {
    auto it = values.find(key);
    if (it != values.end()) {
        std::string val = it->second;
        if (val == "true" || val == "1" || val == "yes") {
            return true;
        } else if (val == "false" || val == "0" || val == "no") {
            return false;
        }
        LOG_WARNING("Invalid bool value for key: " + key);
    }
    return defaultValue;
}

void Config::setString(const std::string& key, const std::string& value) {
    values[key] = value;
}

void Config::setInt(const std::string& key, int value) {
    values[key] = std::to_string(value);
}

void Config::setFloat(const std::string& key, float value) {
    values[key] = std::to_string(value);
}

void Config::setBool(const std::string& key, bool value) {
    values[key] = value ? "true" : "false";
}
