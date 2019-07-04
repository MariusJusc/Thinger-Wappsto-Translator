#pragma once

#include <variant/variant.hpp>
#include <regex>
#include <string>
#include "error.hpp"
#include "util.hpp"

using namespace mapbox;

namespace qplus {

    typedef struct DeviceUrl {
        std::string networkId;
        std::string deviceId;
    } DeviceUrl_t;

    typedef struct ValueUrl {
        std::string networkId;
        std::string deviceId;
        std::string valueId;
    } ValueUrl_t;

    typedef struct StateUrl {
        std::string networkId;
        std::string deviceId;
        std::string valueId;
        std::string stateId;
    } StateUrl_t;
    
    typedef struct StatusUrl {
        std::string networkId;
        std::string deviceId;
        std::string statusId;
    } StatusUrl_t;


    using Url = util::variant<DeviceUrl_t, ValueUrl_t, StateUrl_t, StatusUrl_t>;
 
    Url parseUrl(const std::string& urlstring)
    {
        auto vect = split(urlstring);
        if (vect.size() < 2) {
            throw error_t("Not valid url format: " + urlstring);
        }
        std::string lastItem = vect.back();
 
        std::regex re("[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}");
        std::smatch m;
       
        Url url;
        if (lastItem == "device" && vect.size() == 3 ) { // network/uuid/device
            url = DeviceUrl_t{vect[1], ""};
        } 
        else if (lastItem == "value" && vect.size() == 5) { // network/uuid/device/uuid/value
            url = ValueUrl_t{vect[1], vect[3], ""};
        }
        else if (lastItem == "status" && vect.size() == 5) { // network/uuid/device/status
            url = StatusUrl_t{vect[1], vect[3], ""};
        }
        else if (lastItem == "state" && vect.size() == 7) { // network/uuid/device/uuid/value/uuid/state
            url = StateUrl_t{vect[1], vect[3], vect[5], ""};
        }
        else if (std::regex_match(lastItem, m, re)) {
            lastItem = vect.end()[-2];
            if (lastItem == "device" && vect.size() == 4) { // network/uuid/device/uuid 
                url = DeviceUrl_t{vect[1], vect[3]};
            }
            else if (lastItem == "value" && vect.size() == 6) { // network/uuid/device/uuid/value/uuid
                url = ValueUrl_t{vect[1], vect[3], vect[5]};
            }
            else if (lastItem == "status" && vect.size() == 6) { // network/uuid/device/uuid/status/uuid
                url = StatusUrl_t{vect[1], vect[3], vect[5]};
            }
            else if (lastItem == "state" && vect.size() == 8) { // network/uuid/device/uuid/value/uuid/state/uuid
                url = StateUrl_t{vect[1], vect[3], vect[5], vect[7]};
            }
            else {
                throw error_t("Not valid url format: " + urlstring);
            }
        }
        else {
            throw error_t("Not valid url format: " + urlstring);
        }

        return url;
    }
   
} // namespace qplus
