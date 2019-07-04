#include "state.hpp"
#include "value.hpp"
#include "serializer.hpp"
#include "path.hpp"

namespace qplus {
    
    inline bool exist(const json& j, const std::string& key) {
        if (j.find(key) != j.end()) return true; else return false;
    }

    void to_json(json& j, const StateDesc& s)
    {
        j = json {
            {":id", s.id},
            {":type", s.type},    
            {"data", s.data},
            {"timestamp", s.timestamp},
            {"type", s.typee},
            {"status", s.status}
        };
    }

    void from_json(const json& j, StateDesc& s)
    {
        j.at(":id").get_to(s.id);
        j.at(":type").get_to(s.type);
        j.at("data").get_to(s.data);
        j.at("timestamp").get_to(s.timestamp);
        j.at("type").get_to(s.typee);
        exist(j, "status") ? j["status"].get_to(s.status) : s.status = "Send"; 
    }

    State::State(Value* parent, const std::string& uuid) : parent_(parent), netid(parent->netid)
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
            desc = StateDesc{}; // make a default
        }
        try { deserialize(filename() + ".box", box); }
        catch(std::exception& e) { // just report 
        }
    }

    std::string State::location() const
    {
        return parent_->location() + parent_->desc.id + "/state/";
    }

    std::string State::filename() const 
    {
        return this->location() + desc.id + "/state.json";
    }

} // namespace 
