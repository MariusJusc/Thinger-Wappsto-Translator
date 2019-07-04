#pragma once

#include <string>
#include <vector>
#include <memory>
#include "network.hpp"
#include "configuration.hpp"

using json = nlohmann::json;

namespace qplus {

    struct RootDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:network-1.1";
        std::string name = "Seluxit NET";
    };

    class Root {
        private:
            Conf_t conf_;

        public:
            Root(const Conf_t conf);
            ~Root();

            std::vector<std::unique_ptr<Network>> networks;
            std::string location() const;

        public:
            std::string netid;
            RootDesc desc;
            json box;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const RootDesc& network);
    void from_json(const json& j, RootDesc& n);

} // namespace root
