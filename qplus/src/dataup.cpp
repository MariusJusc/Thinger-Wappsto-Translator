#include "dataup.hpp"

#include "ventilator.hpp"
#include "util.hpp"
#include "network.hpp"
#include "device.hpp"
#include "value.hpp"
#include "state.hpp"
#include "root.hpp"
#include "serializer.hpp"
#include "url.hpp"
#include <regex>
#include <random>

namespace qplus {
    
    bool exist(const json& j, const std::string& key) 
    {
        if (j.find(key) != j.end()) return true; else return false;
    }

    bool isPost(const json& ajson) 
    {
        if (!exist(ajson, "method"))
            return false;

        return ajson["method"] == "POST";
    }
    
    bool isPut(const json& ajson)
    {
        if (!exist(ajson, "method"))
            return false;
        
        return ajson["method"] == "PUT";
    }

    bool isDelete(const json& ajson) 
    {
        if (!exist(ajson, "method"))
            return false;

        return ajson["method"] == "DELETE";
    }
    
    std::string randomize(const std::string& optional_str)
    {
        std::random_device rd;
        std::mt19937 engine(rd());
        std::uniform_int_distribution<long> distribution(100000, 999999);
        
        if (!optional_str.empty()) {
            return optional_str + "-" + std::to_string(distribution(engine)); 
        }
        else {
            return std::to_string(distribution(engine));
        }
    }

    bool deleteItem(const json& ajson, bool backend) {

        std::string url = ajson["/params/url"_json_pointer];
        auto v = split(url);
        if (v.size() < 2)
            return false;

        std::string uuid = split(url).end()[-1];
        std::string itemName = split(url).end()[-2];
            
        Ventilator* vent = Ventilator::instance();   	

        if (itemName == "network") {
            Network* net = vent->findNetwork(uuid);
            removeEnd(net, backend);
            removeDir(net->location());						
            //delete last.
            removeFromList(vent->root->networks, net);
         }
         else if (itemName == "device") {
            Network* net; Device* dev;
            std::tie(net, dev) = vent->findDevice(uuid);
            if (!dev) {
                vent->logger()->warn("Can not find device " + uuid);
                return false;
            }
            removeEnd(dev, backend);
            removeDir(dev->location());
            // delete last.
            vent->logger()->warn("Deleting device " + dev->desc.id);
            removeFromList(net->devices, dev);
         }
         else if (itemName == "value") {
            Device* dev; Value* val;
            std::tie(dev, val) = vent->findValue(uuid);
            if (!val) {
                vent->logger()->warn("Can not find value " + uuid);
                return false;
            }
            removeEnd(val, backend);
            removeDir(val->location());

            vent->logger()->warn("Deleting value " + val->desc.id);
            // delete last
            removeFromList(dev->values, val);
         }
         else if (itemName == "state") {
            Value* val; State* st;
            std::tie(val, st) = vent->findState(uuid);
            if (!st) {
                vent->logger()->warn("Can not find state " + uuid);
                return false; 
            }
            removeEnd(st, backend);
            removeDir(st->location());
            // delete last
            vent->logger()->warn("Deleting state " + st->desc.id);
            removeFromList(val->states, st);
         }

         return true;
    }
    
    // client -> backend == true, the do not set 'updated' to true, only 
    Device* updateDevice(const DeviceUrl_t& d, const json& ajson, bool backend) 
    {
        Ventilator* vent = Ventilator::instance();

        std::string uuid = ajson["/params/data/:id"_json_pointer];
        DeviceDesc desc = ajson["/params/data"_json_pointer];

        Network* net; Device* dev;
        std::tie(net, dev) = vent->findDevice(uuid);
        if (!dev) {
            net = vent->findNetwork(d.networkId);
            if (net) {
                vent->logger()->info("Creating new device: " + uuid);
                auto device = std::unique_ptr<Device>(new Device(net, uuid));
                net->devices.push_back(std::move(device));
                dev = net->devices.back().get();
            }
            else throw error_t("Can not find network with id: " + d.networkId);
        }
        
        vent->logger()->info("Received device info: " + desc.name);
        if (dev->desc != desc) {
            dev->desc = desc;
            vent->logger()->info("Serializing device: " + dev->filename());
            serialize(desc, dev->filename());
            if (!backend)
                dev->updated = true;
        }

        if (exist(ajson["params"], "box")) {
            json aj = json::parse(ajson["params"]["box"].get<std::string>());
            if (aj != dev->box) { 
                vent->logger()->info("Serializing device: " + dev->filename() + " internal variables - boxing");
                std::string filename = dev->filename() + ".box";
                serialize(aj, filename);             
                dev->box = aj;
            }
        }
       
        return dev;
    }
    
    Value* updateValue(const ValueUrl_t& v, const json& ajson, bool backend)
    {
        Ventilator* vent = Ventilator::instance();
        
        std::string uuid = ajson["/params/data/:id"_json_pointer];
        ValueDesc desc = ajson["/params/data"_json_pointer];
        
        Device* dev; Value* val;
        std::tie(dev, val) = vent->findValue(uuid);
        if (!val) {
            Network* net;
            std::tie(net, dev) = vent->findDevice(v.deviceId);
            if (dev) {
                vent->logger()->info("Creating new Value: " + uuid);
                auto value = std::unique_ptr<Value>(new Value(dev, uuid));
                dev->values.push_back(std::move(value));
                val = dev->values.back().get();
            }
            else throw error_t("Can not find device with id: " + v.deviceId);
        }
        
        vent->logger()->info("Received value info: " + desc.name);
        if (val->desc != desc) {
            vent->logger()->info("Serializing value: " + val->filename());
            val->desc = desc;
            serialize(desc, val->filename());
            if (!backend)
                val->updated = true;
        }
        
        if (exist(ajson["params"], "box")) {
            json aj = json::parse(ajson["params"]["box"].get<std::string>());
            if (aj != val->box) {
                vent->logger()->info("Serializing value: " + val->filename() + " internal variables - boxing");
                std::string filename = val->filename() + ".box";
                serialize(aj, filename);             
                val->box = aj;
            }
        }

        return val;
    }
    
    State* updateState(const StateUrl_t& s, const json& ajson, bool backend) 
    {
        Ventilator* vent = Ventilator::instance();

        StateDesc desc = ajson["/params/data"_json_pointer];
        std::string uuid = ajson["/params/data/:id"_json_pointer];

        Value* val; State* st;
        std::tie(val, st) = vent->findState(uuid);
        if (!st) {
            Device* dev;
            std::tie(dev, val) = vent->findValue(s.valueId);
            if (val) {
                vent->logger()->info("Creating new State: " + uuid);
                auto state = std::unique_ptr<State>(new State(val, uuid));
                val->states.push_back(std::move(state));
                st = val->states.back().get();
            }
            else throw error_t("Can not find value with id: " + s.valueId);
        }

        vent->logger()->info("Received state data: " + desc.data);
        if (st->desc != desc) {
            st->desc = desc;
            vent->logger()->info("Serializing state: " + st->filename());
            serialize(desc, st->filename());
            if (!backend)
                st->updated = true;
        }

        if (exist(ajson["params"], "box")) {
            json aj = json::parse(ajson["params"]["box"].get<std::string>());
            if (aj != st->box) {
                vent->logger()->info("Serializing device: " + st->filename() + " internal variables - boxing");
                std::string filename = st->filename() + ".box";
                serialize(aj, filename);             
                st->box = aj;
            }
        }

        return st;
    }

    Status* updateStatus(const StatusUrl_t& s, const json& ajson, bool) 
    {
        Ventilator* vent = Ventilator::instance();

        StatusDesc desc = ajson["/params/data"_json_pointer];
        std::string uuid = ajson["/params/data/:id"_json_pointer];

        vent->logger()->info("Updating status type: " + desc.typee + " level: " + desc.level);
        
        // find device, create new status.
        Network* net = nullptr; Device* dev = nullptr; 
        Status* st = nullptr;
        std::tie(net, dev) = vent->findDevice(s.deviceId);
        if (dev) {
            auto status = std::unique_ptr<Status>(new Status(dev, uuid));
            status->desc = desc;
            dev->statuses.push_back(std::move(status));
            st = dev->statuses.back().get();
            if (dev->statuses.size() > 50) {
                dev->statuses.pop_front();
            }
        }
        else throw error_t("Can not find device with id: " + s.deviceId);

        return st;
    }

    bool updateItem(const json& ajson, bool backend) 
    {
        std::string urlstring = ajson["/params/url"_json_pointer];
        std::string method = ajson.at("method"); 

        // Make sure it follows pattern /network/uuid/device/uuid/value/uuid/state
        Url url = parseUrl(urlstring);
        
        // postTo function will reset 'updated' state if msg send from client to backed.
        url.match( [&](DeviceUrl_t d)  {
            Device* dev = updateDevice(d, ajson, backend);
            postTo(method, backend, dev);
        }
        , [&] (ValueUrl_t v) {
            Value* val = updateValue(v, ajson, backend);
            postTo(method, backend, val);

        }
        , [&] (StateUrl_t s) {
            State* st = updateState(s, ajson, backend);
            postTo(method, backend, st);
        }
        , [&] (StatusUrl_t s) {
            Status* st = updateStatus(s, ajson, backend);
            postTo(method, backend, st);
        }
        );

        return true;
    }

    json createJsonRpc(const std::string& method, const json& data, const std::string& url) 
    {
        std::string rpcid = randomize(method);

        json params;
        params["url"] = url;
        params["data"] = data;

        json value;
        value["jsonrpc"] = "2.0";
        value["id"] = rpcid;
        value["method"] = method;
        value["params"] = params;

        return value;
    }

    std::string getUrl(const std::string& location, bool forBackend)  
    {
        std::size_t found = location.find("/network/");
        if (found == std::string::npos)
            return "";
        
        if (!forBackend) {
            return location.substr(found);
        }

        auto v = split(location.substr(found));
        // Replace Network UUID with Root UUID
        if (v.size() > 1) {
            v[1] = Ventilator::instance()->conf().QW_UUID;
        }

        return concat(v);
    }
    
    json responseMsg(const std::string& id) 
    {
        json ret = json {
            {"id", id},
            {"jsonrpc", "2.0"},
            {"result", true}
        };
        return ret;
    }

    json errorMsg(const std::string& id, const std::string& msg) 
    {
        json error = json {
            {"message", msg},
            {"code", -32001}
        };
        json ret  = json {
              {"id", id}, 
              {"jsonrpc", "2.0"}, 
              {"error", error}
        };

        return ret;
    }

} // namespace qplus
