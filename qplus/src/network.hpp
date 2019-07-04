#pragma once

#include <string>
#include <vector>
#include <memory>
#include "json.hpp"
#include "device.hpp"
#include "configuration.hpp"

using json = nlohmann::json;

namespace qplus {

    class  Root;

    struct NetworkDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:network-1.1";
        std::string name;
    };

    class Network {
        private:
            Root* parent_;

        public:
            Network(Root* parent, const std::string& uuid);
            ~Network();

            std::vector<std::unique_ptr<Device>> devices;
            std::string location() const;

        public:    
            std::string netid;
            NetworkDesc desc;
            json box;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const NetworkDesc& network);
    void from_json(const json& j, NetworkDesc& n);

} // namespace qplus
