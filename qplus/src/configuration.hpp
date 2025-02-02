#pragma once

#include <fstream>
#include <string>
#include "json.hpp"
#include "error.hpp"

typedef struct Conf {
    
    std::string QPLUS_CONF;
    std::string QPLUS_PATH;
    std::string SERVER;
    std::string CLIENT_CRT;
    std::string CLIENT_KEY;
    std::string CA_CRT;
    std::string QW_UUID;
    int LISTENING_PORT;

} Conf_t;

static Conf_t readConfiguration()
{
    using json = nlohmann::json;

    // TODO: Change PATH!!!
    Conf_t conf;
    conf.QPLUS_CONF = "/home/mariusjusc/Desktop/cpp_proj/Thinger Project/qplus/src/qplus.conf";
    conf.QPLUS_PATH   = "/home/mariusjusc/Desktop/cpp_proj/Thinger Project/qplus";

    std::ifstream is(conf.QPLUS_CONF.c_str()); 
    if (!is.good()) {
        throw qplus::error_t("Can't open configuration file: " + conf.QPLUS_CONF);
    }
   
    json object_json;
    is >> object_json;
    
    conf.QW_UUID        = object_json["QW_UUID"].get<std::string>();
    if (conf.QW_UUID.empty()) {
        throw qplus::error_t("Provide GATEWAY UUID as 'QW_UUID' - it should not be empty!");
    }

    conf.SERVER         = object_json["SERVER"].get<std::string>();
    conf.CLIENT_CRT     = object_json["CLIENT_CRT"].get<std::string>();
    conf.CLIENT_KEY     = object_json["CLIENT_KEY"].get<std::string>();
    conf.CA_CRT         = object_json["CA_CRT"].get<std::string>();
    conf.LISTENING_PORT = object_json["LISTENING_PORT"].get<int>();

    return conf;
}

