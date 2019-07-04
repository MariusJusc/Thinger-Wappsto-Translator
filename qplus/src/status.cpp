#include "status.hpp"
#include "device.hpp"
#include "path.hpp"


namespace qplus {
    
    inline bool exist(const json& j, const std::string& key) {
        if (j.find(key) != j.end()) return true; else return false;
    }

    void to_json(json& j, const StatusDesc& s)
    {
        j = json {
            {":id", s.id},
            {":type", s.type},    
            {"message", s.message},
            {"timestamp", s.timestamp},
            {"type", s.typee},
            {"level", s.level}
        };
    }

    void from_json(const json& j, StatusDesc& s)
    {
        j.at(":id").get_to(s.id);
        j.at(":type").get_to(s.type);
        j.at("message").get_to(s.message);
        j.at("timestamp").get_to(s.timestamp);
        j.at("type").get_to(s.typee);
        exist(j, "level") ? j["level"].get_to(s.level) : s.level= ""; 
    }

    Status::Status(Device* parent, const std::string& uuid) : parent_(parent), netid(parent->netid)
    {
        desc.id = uuid;
    }

    std::string Status::location() const
    {
        return parent_->location() + parent_->desc.id + "/status/";
    }

}
