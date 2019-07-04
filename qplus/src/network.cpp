#include "network.hpp"

#include "configuration.hpp"
#include "path.hpp"
#include "root.hpp"

namespace qplus {

    inline bool exist(const json& j, const std::string& key) {
        if (j.find(key) != j.end()) return true; else return false;
    }

    void to_json(json& j, const NetworkDesc& n) {
        j = json {
            {"name", n.name}, 
            {":type", n.type}, 
            {":id", n.id}
        };
    }

    void from_json(const json& j, NetworkDesc& n) {
        // required., will trow exceptyion if not exist.
        j.at(":id").get_to(n.id);
        j.at(":type").get_to(n.type);
        // optional
        exist(j, "name") ? j["name"].get_to(n.name) : n.name = ""; 
    }

    Network::Network(Root* parent, const std::string& uuid) : parent_(parent), netid(uuid)
    {
        desc.id = uuid;
        std::string dir = location() + desc.id;
		if (!Path::exists(dir)) {
            Path::mkdirectory(dir);
			return;
		} 
        
        dir += "/device";
        if (!Path::exists(dir))
            return;
        
        Path path(dir);
        while (path.directory() != path.dirs_end()) {
            
            auto device = std::unique_ptr<Device>(new Device(this, *path.directory()));
            devices.push_back(std::move(device));
            path.directory()++;
        }
    }
    
    Network::~Network()
    {
        devices.clear();
    }

    std::string Network::location() const
    {
        return parent_->location();
    }
    
} // namespace qplus
