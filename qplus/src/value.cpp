#include "value.hpp"
#include "device.hpp"
#include "path.hpp"
#include <string>
#include "serializer.hpp"
#include <iostream>

namespace qplus {

    inline bool exist(const json& j, const std::string& key) {
        if (j.find(key) != j.end()) return true; else return false;
    }

    void to_json(json& j, const ValueDesc& v)
    {
        j = json {
            {":id", v.id},
            {":type", v.type},    
            {"name", v.name}, 
            {"permission", v.permission}, 
            {"type", v.typee}, 
            {"status", v.status} 
        };
        if (v.number.exist) {
            j["number"] = { 
                    {"min", v.number.min}, 
                    {"max", v.number.max}, 
                    {"step", v.number.step},            
                    {"unit", v.number.unit}            
                };
        }
        else if (v.string.exist) {
            j["string"] = { 
                    {"max", v.string.max}, 
                    {"encoding", v.string.encoding}            
                };
        }
        else if (v.blob.exist) {
            j["blob"] = { 
                    {"max", v.blob.max}, 
                    {"encoding", v.blob.encoding}            
                };
        }
        else {}
    }

    void from_json(const json& j, ValueDesc& v)
    {
        j.at(":id").get_to(v.id);
        j.at(":type").get_to(v.type);
        j.at("permission").get_to(v.permission);

        exist(j, "name") ? j["name"].get_to(v.name) : v.name = ""; 
        exist(j, "type") ? j["type"].get_to(v.typee) : v.typee = ""; 
        exist(j, "status") ? j["status"].get_to(v.status) : v.status = ""; 

        if (exist(j, "number")) {
            j["number"].at("min").get_to(v.number.min);
            j["number"].at("max").get_to(v.number.max);
            j["number"].at("step").get_to(v.number.step);
            exist(j["number"], "unit") ? j["number"]["unit"].get_to(v.number.unit) : v.number.unit = ""; 
            v.number.exist = true;
        }
        else if (exist(j, "string")) {
            j["string"].at("max").get_to(v.string.max);
            exist(j["string"], "encoding") ? j["string"]["encoding"].get_to(v.string.encoding) : v.string.encoding= ""; 
            v.string.exist = true;
        }
        else if (exist(j, "blob")) {
            j["blob"].at("max").get_to(v.blob.max);
            exist(j["blob"], "encoding") ? j["blob"]["encoding"].get_to(v.blob.encoding) : v.blob.encoding= ""; 
            v.blob.exist = true;
        }
    }

    Value::Value(Device* parent, const std::string& uuid) : parent_(parent), netid(parent->netid)
    {
        desc.id = uuid;

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
            desc = ValueDesc{}; // make a default
            return;
        }
        try { deserialize(filename() + ".box", box); }
        catch(std::exception& e) { // just report 
        }

        //std::cout << "box " << uuid << " " << box.dump() << "\n";
        dir += "/state";

        // check if there is a values
        Path path(dir);
        while (path.directory() != path.dirs_end()) {
            
            auto state = std::unique_ptr<State>(new State(this, *path.directory()));
            states.push_back(std::move(state));
            path.directory()++;
        }
    }
    
    Value::~Value()
    {
        states.clear();
    }

    std::string Value::filename() const 
    {
        return location() + desc.id + "/value.json";
    }

    std::string Value::location() const
    {
        return parent_->location() + parent_->desc.id + "/value/";
    }

} // namespace 
