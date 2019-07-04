#pragma once

#include "json.hpp"
#include <string>
#include <iostream>
#include "ventilator.hpp"
#include "backendsocket.hpp"

using json = nlohmann::json;

namespace qplus {
    
    // For RECEIVE
    bool exist(const json& j, const std::string& key);

    bool isPost(const json& ajson);
    bool isPut(const json& ajson);
    bool isDelete(const json& ajson);

    bool deleteItem(const json& ajson, bool backend);
    bool updateItem(const json& ajson, bool backend);

    // For SEND
    json errorMsg(const std::string& id, const std::string& msg);
    json responseMsg(const std::string& id);

    json createJsonRpc(const std::string& method, const json& root, const std::string& url);
    std::string getUrl(const std::string& location, bool forBackend);
    
    template<typename S, typename T>
    void post(const std::string& method, const S* socket, const T* object)
    {
        std::string url;
        if (method == "POST") {
            url = getUrl(object->location(), socket->isBackend());
        }
        else { // GET DELETE PUT
            url = getUrl(object->location() + object->desc.id, socket->isBackend());
        }
        json ajson;
        to_json(ajson, object->desc);
        json payload = createJsonRpc(method, ajson, url);

        socket->write(payload);            
 
        // BLOCKING read - just for test. Using async callback instead.
/*
 *        std::string response = socket->read();
 *        if (socket->isBackend())
 *            vent->logger()->info(" <--- from Bastard " + response + "\n");
 *        else 
 *            vent->logger()->info(" ---> from Client " + response + "\n");
 */
    }
    

    template<typename S, typename T>
    void _remove(const S* socket, const T* object)
    {
        // getUrl removes last 'backslash' 
        std::string url = getUrl(object->location(), socket->isBackend());
        url += "/" + object->desc.id;
        json ajson{};
        json payload = createJsonRpc("DELETE", ajson, url);
        socket->write(payload);   
        Ventilator::instance()->logger()->info(payload.dump());
    }
    
    template<typename T>
    void removeEnd(const T* object, bool fromBackend)
    {
        if (!object) {
            throw error_t("Null object");   
        }

        Ventilator* vent = Ventilator::instance();
        if (object && fromBackend) {
            _remove(vent->bSocket(), object);
            return;
        }
        
        // remove from client (front)
        const Client* cl = vent->findClient(object->netid);                                        
        if (cl) {
           _remove(cl, object);
        }
        else 
            throw error_t("Can not find client with id: " + object->netid); 
    }
    
    template<typename T>
    void postTo(const std::string& method, bool backend, const T* item )
    {
        Ventilator* vent = Ventilator::instance();
        if (backend) {
            post(method, vent->bSocket(), item);
        }
        else {
            const Client* cl = vent->findClient(item->netid);                                        
            if (cl)
                cl->checkUpdates();
            else throw error_t("Can not find client with id: " + item->netid);
        }
    }

} // namespace
