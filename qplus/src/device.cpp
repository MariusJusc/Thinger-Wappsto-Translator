#include "device.hpp"
#include "network.hpp"
#include "path.hpp"
#include "serializer.hpp"
#include "ventilator.hpp"
#include <iostream>

namespace qplus {

    inline bool exist(const json& j, const std::string& key) 
    {
        if (j.find(key) != j.end()) return true; else return false;
    }

    void to_json(json& j, const DeviceDesc& d)
    {
        j = json {
            {":id", d.id},
            {":type", d.type},    
            {"name", d.name}, 
            {"manufacturer", d.manufacturer},
            {"product", d.product},
            {"version", d.version},
            {"serial", d.serial},
            {"protocol", d.protocol},
            {"communication", d.communication},
            {"included", d.included},
            {"command", d.command}
        };
    }

    void from_json(const json& j, DeviceDesc& d)
    {
        j.at(":id").get_to(d.id);
        j.at(":type").get_to(d.type);
       
        exist(j, "name") ? j["name"].get_to(d.name) : d.name = ""; 
        exist(j, "manufacturer") ? j["manufacturer"].get_to(d.manufacturer) : d.manufacturer = ""; 
        exist(j, "product") ? j["product"].get_to(d.product) : d.product = ""; 
        exist(j, "version") ? j["version"].get_to(d.version) : d.version = ""; 
        exist(j, "serial") ? j["serial"].get_to(d.serial) : d.serial = ""; 
        exist(j, "protocol") ? j["protocol"].get_to(d.protocol) : d.protocol = ""; 
        exist(j, "communication") ? j["communication"].get_to(d.communication) : d.communication = ""; 
        exist(j, "included") ? j["included"].get_to(d.included) : d.included = "0"; 
        exist(j, "command") ? j["command"].get_to(d.command) : d.command= "idle"; 
    }

    Device::Device(Network* parent, const std::string& uuid) : parent_(parent), netid(parent->desc.id)
    {
        desc.id = uuid;
        // If not exist - create.
        if (!Path::exists(location())) {
            Path::mkdirectory(location()); 
        }
        
        std::string dir = location() + desc.id;
		if (!Path::exists(dir)) {
            Path::mkdirectory(dir);
            Path::mkfile(filename());
			return;
		} 
        
        // If exist load from file.
        try { deserialize(filename(), desc); }
        catch(std::exception& e) {
            // spd log.
            desc = DeviceDesc{}; // make a default
            return;
        }
        try { deserialize(filename() + ".box", box); }
        catch(std::exception& e) { // just report 
        }
        
        dir += "/value";
        if (!Path::exists(dir))
            return;

        // check if there is a values
        Path path(dir);
        while (path.directory() != path.dirs_end()) {
            
            auto value = std::unique_ptr<Value>(new Value(this, *path.directory()));
            values.push_back(std::move(value));
            path.directory()++;
        }
    }
    
    Device::~Device()
    {
        values.clear();
    }

    std::string Device::filename() const 
    {
        return this->location() + desc.id + "/device.json";
    }

    std::string Device::location() const
    {
        return parent_->location() + parent_->desc.id + "/device/";
    }
}
