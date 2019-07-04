#pragma once

#include <string>
#include <memory>
#include "json.hpp"
#include "state.hpp"

using json = nlohmann::json;

namespace qplus {
   
    class Device;
    
    struct ValueNumber {
        double min;
        double max;
        double step;
        std::string unit;
        bool exist = false;
        
        bool operator==(const ValueNumber& other) const
        {
            return this->min == other.min &&
                   this->max == other.max &&
                   this->step == other.step &&
                   this->unit == other.unit &&
                   this->exist == other.exist;
        }
    };
    
    struct ValueSet {
        std::string element;
        bool exist = false;
    };

    struct ValueString {
        unsigned int max;
        std::string encoding;
        bool exist = false; 
        bool operator==(const ValueString& other) const
        {
            return this->max == other.max &&
                   this->encoding == other.encoding &&
                   this->exist == other.exist;
        }
    };

    struct ValueBlob {
        unsigned int max;
        std::string encoding;
        bool exist = false;

        bool operator==(const ValueBlob& other) const
        {
            return this->max == other.max &&
                   this->encoding == other.encoding &&
                   this->exist == other.exist;
        }
    };
    

    struct ValueDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:value-1.1";
        std::string name;
        std::string permission;
        std::string typee;
        std::string status;

        ValueNumber number;
        ValueString string;
        ValueBlob blob;

        ValueDesc& operator=(const ValueDesc& other)
        {
            if (!other.id.empty())
                this->id = other.id;
            if (!other.type.empty())
                this->type = other.type;
            if (!other.name.empty())
                this->name = other.name;
            if (!other.permission.empty())
                this->permission = other.permission;
            if (!other.typee.empty())
                this->typee = other.typee;
            if (!other.status.empty())
                this->status = other.status;
            
            this->number = other.number;
            this->string = other.string;
            this->blob = other.blob;

            return *this;
        }

        ValueDesc(const ValueDesc& other)
        {
            *this = other;
        }
    
        bool operator==(const ValueDesc& other) const
        {
            return this->id == other.id &&
                this->type == other.type &&
                this->name == other.name &&
                this->permission == other.permission &&
                this->typee == other.typee &&
                this->status == other.status &&
                this->number == other.number &&
                this->string == other.string &&
                this->blob == other.blob;
        }
        
        bool operator!=(const ValueDesc& other) const
        {
            return !(*this == other);
        }

        ValueDesc() {}
    };
    

    class Value {
        private:
            Device* parent_;

        public:
            Value(Device* parent, const std::string& uuid);
            ~Value();

            std::vector<std::unique_ptr<State>> states;
            std::string location() const;
            std::string filename() const;
            
        public:
            std::string netid;
            ValueDesc desc;
            json box;
            bool updated = false;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const ValueDesc& v);
    void from_json(const json& j, ValueDesc& v);

} // namespace qplus
