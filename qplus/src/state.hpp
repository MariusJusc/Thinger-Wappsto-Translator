#pragma once

#include <string>
#include "json.hpp"

using json = nlohmann::json;    

namespace qplus {
    
    class Value;

    struct StateDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:state-1.1";
        std::string timestamp;
        std::string data;
        std::string typee;
        std::string status;

        StateDesc& operator=(const StateDesc& other)
        {
            if (!other.id.empty())
                this->id = other.id;
            if (!other.type.empty())
                this->type = other.type;
            if (!other.timestamp.empty())
                this->timestamp = other.timestamp;
            if (!other.data.empty())
                this->data = other.data;
            if (!other.typee.empty())
                this->typee = other.typee;
            if (!other.status.empty())
                this->status = other.status;
            
            return *this;
        }

        StateDesc(const StateDesc& other)
        {
            *this = other;
        }
    
        bool operator==(const StateDesc& other) const
        {
            return this->id ==  other.id &&
                this->type == other.type &&
                this->timestamp == other.timestamp &&
                this->data == other.data && 
                this->typee == other.typee &&
                this->status == other.status;
        }
        
        bool operator!=(const StateDesc& other) const
        {
            return !(*this == other);
        }

        StateDesc() {}

    };

    class State {
        private:
            Value* parent_;

        public:
            State(Value* parent, const std::string& uuid);
            std::string location() const;
            std::string filename() const;
            
        public:
            std::string netid;
            StateDesc desc;
            json box;
            bool updated = false;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const StateDesc& s);
    void from_json(const json& j, StateDesc& s);

} // namespace


