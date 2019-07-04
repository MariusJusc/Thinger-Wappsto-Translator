#pragma once

#include <string>
#include <vector>
#include <list>
#include <memory>
#include "value.hpp"
#include "status.hpp"
#include "json.hpp"

using json = nlohmann::json;    

namespace qplus {

    class Network;

    struct DeviceDesc {
        std::string id;
        std::string type = "urn:seluxit:xml:bastard:device-1.1";
        std::string name;
        std::string manufacturer;
        std::string product;
        std::string version;
        std::string serial;
        std::string protocol;
        std::string communication;
        std::string included;
        std::string command;

        DeviceDesc& operator=(const DeviceDesc& other)
        {
            if (!other.id.empty())
                this->id = other.id;
            if (!other.type.empty())
                this->type = other.type;
            if (!other.name.empty())
                this->name = other.name;
            if (!other.manufacturer.empty())
                this->manufacturer = other.manufacturer;
            if (!other.product.empty())
                this->product = other.product;
            if (!other.version.empty()) 
                this->version = other.version;
            if (!other.serial.empty())
                this->serial = other.serial;
            if (!other.protocol.empty())
                this->protocol = other.protocol;
            if (!other.communication.empty())
                this->communication = other.communication;
            if (!other.included.empty())
                this->included = other.included;
            if (!other.command.empty()) 
                this->command = other.command;
            return *this;
        }
 
        DeviceDesc(const DeviceDesc& other)
        {
            *this = other;
        }
       
        bool operator==(const DeviceDesc& other) const 
        {
            return this->id == other.id &&
                this->type == other.type && 
                this->name == other.name &&
                this->manufacturer == other.manufacturer &&
                this->product == other.product &&
                this->version == other.version &&
                this->serial == other.serial &&
                this->protocol == other.protocol &&
                this->communication == other.communication &&
                this->included == other.included &&
                this->command == other.command;
        }
        
        bool operator!=(const DeviceDesc& other) const 
        {
            return !(*this == other);
        }

        DeviceDesc() {}
    };

    class Device {
        private:
            Network* parent_;
       
        public:
            Device(Network* parent, const std::string& uuid);
            ~Device();

            std::vector<std::unique_ptr<Value>> values;
            std::list<std::unique_ptr<Status>> statuses;
            std::string location() const;
            std::string filename() const;

        public: 
            std::string netid;
            DeviceDesc desc;
            json box;
            bool updated = false;
    };

    // nlohman json arbitrary types conversion
    void to_json(json& j, const DeviceDesc& d);
    void from_json(const json& j, DeviceDesc& d);

} // namespace qplus
