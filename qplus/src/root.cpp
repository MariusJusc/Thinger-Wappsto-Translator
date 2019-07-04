#include "root.hpp"
#include "path.hpp"
#include "util.hpp"

namespace qplus {

    void to_json(json& j, const RootDesc& n) {
        j = json {
            {"name", n.name}, 
            {":type", n.type}, 
            {":id", n.id}
        };
    }

    void from_json(const json& j, RootDesc& n) {
        j.at(":id").get_to(n.id);
        j.at(":type").get_to(n.type);
        j.at("name").get_to(n.name); 
    }


    Root::Root(const Conf_t conf) : conf_(conf), netid(conf_.QW_UUID) 
    {
        desc.id = conf_.QW_UUID;
        std::string dir = conf_.QPLUS_PATH + "/network";

        if (!Path::exists(dir)) {
            Path::mkdirectory(dir);
            return;
        }

        Path path(dir);
        while (path.directory() != path.dirs_end()) {
            
            auto network = std::unique_ptr<Network>(new Network(this, *path.directory()));
            networks.push_back(std::move(network));
            path.directory()++;
        }
    }
    
    Root::~Root() 
    {
        networks.clear();
    }

    std::string Root::location() const
    {
        return conf_.QPLUS_PATH + "/network/";
    }

} // namespace qplus
