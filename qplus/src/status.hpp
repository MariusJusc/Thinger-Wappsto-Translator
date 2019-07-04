#pragma once

#include <string>
#include "json.hpp"

using json = nlohmann::json;    

namespace qplus {
    
    class Device;

    struct StatusDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:status-1.1";
        std::string timestamp;
        std::string level;
        std::string typee;
        std::string message;

        StatusDesc& operator=(const StatusDesc& other)
        {
            if (!other.id.empty())
                this->id = other.id;
            if (!other.type.empty())
                this->type = other.type;
            if (!other.timestamp.empty())
                this->timestamp = other.timestamp;
            if (!other.message.empty())
                this->message = other.message;
            if (!other.typee.empty())
                this->typee = other.typee;
            if (!other.level.empty())
                this->level = other.level;
            
            return *this;
        }

        StatusDesc() {}

        StatusDesc(const StatusDesc& other)
        {
            *this = other;
        }
    };

    class Status {
        private:
            Device* parent_;

        public:
            Status(Device* parent, const std::string& uuid);
            std::string location() const;
            
        public:
            std::string netid;
            StatusDesc desc;
            bool updated = false;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const StatusDesc& s);
    void from_json(const json& j, StatusDesc& s);

} // namespace

