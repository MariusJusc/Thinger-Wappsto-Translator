#pragma once

#include <string>
#include <fstream>
#include "json.hpp"
#include "error.hpp"
#include <stdio.h>

using json = nlohmann::json;

namespace qplus {

    template<typename T>
    void serialize(const T& object, const std::string& filename)
    {
        std::string tmp = filename + ".tmp"; 
        std::ofstream os(tmp.c_str(), std::ios::out);
        if (!os) {
            throw error_t("Can't open file: " + tmp + " for writing.");
        }
        try {
            json j;
            to_json(j, object);
            os << j.dump();
            
            rename(tmp.c_str(), filename.c_str());
        }
        catch(std::exception& e) {
            os.close();
            throw error_t("Serialization failed: " + filename);
        }
        os.close();
    }
    
    void serialize(const json& ajson, const std::string& filename);

    void deserialize(const std::string& filename, json& ajson);

    template<typename T>
    void deserialize(const std::string& filename, T& object)
    {
        std::ifstream is(filename.c_str()); 
        if (!is.good()) {
            throw error_t("Can't open file: " + filename + " for reading.");
        }
        try {
            json j;
            is >> j;
        
            is.close();
            from_json(j, object);
        }
        catch(std::exception& e) {
            throw error_t("Deserialization failed: " +  filename + " " + e.what());
        }
    }

    int recursiveDelete(const char *dir);
    void removeDir(const std::string& path);

} // namespace qwplus

