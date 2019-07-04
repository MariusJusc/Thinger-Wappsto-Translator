#include "backendsocket.hpp"
#include "sslclient.hpp"
#include "ventilator.hpp"
#include "dataup.hpp"
#include "util.hpp"

const int MSG_LIMIT = 500;

namespace qplus {
    
    BackendSocket::BackendSocket(const Conf_t& conf) : sslclient_(conf.CLIENT_CRT, conf.CLIENT_KEY, conf.CA_CRT),
                                                       endpoint_(conf.SERVER)
    {
        wconnect_.set(0., 30.);
        wconnect_.set<BackendSocket, &BackendSocket::reconnect>(this);
    }
    
    bool BackendSocket::connect()
    {
        bool success = sslclient_.connect(endpoint_);
        Ventilator* vent = Ventilator::instance();
        if (!success) {
            vent->logger()->info("Failed to connect. Reconnect again after [30.0] sec.");
            wconnect_.start();
            return false;
        }
        
        vent->logger()->info("Connection with backend: '" + endpoint_ + "' succeeded!");
        // obligatory.
        vent->registerNetwork();

        bio.set(sslclient_.descriptor(), ev::READ);
        bio.set<BackendSocket, &BackendSocket::readCallback>(this);
        bio.start();

        bastardReady_ = true;
        return true;
    }

    void BackendSocket::reconnect(ev::timer& watcher, int)
    {
        bool success = connect();
        if (!success) {
            Ventilator::instance()->logger()->info("No connection. Reconnect again after [" + std::to_string(watcher.remaining()) + "] sec.");
            return;
        }
    
        wconnect_.stop();
    }

    void BackendSocket::parseJson(const json& aj)
    {
        Ventilator* vent = Ventilator::instance();
        std::string rpcid = aj["id"];
        //
        // Bastard sends a List.
        if (exist(aj, "result") && exist(aj["result"], "value") && exist(aj["result"]["value"], ":child")) {
            
            std::string listOf = aj["result"]["value"][":child"];
            vent->logger()->info(" <--- Bastard response - list of " + listOf);

            // check is list of devices 'id' exist - in case no devices present on openAPI - id tag do not exist in json
            if (listOf == "urn:seluxit:xml:bastard:device-1.1" && exist(aj["result"]["value"], "id")) {
                json alist = aj["result"]["value"]["id"];        
                json data{};
                std::string url = "/network/" + vent->conf().QW_UUID + "/device";
                for (json::iterator it = alist.begin(); it != alist.end(); ++it) {
                    Network* net; Device* dev;
                    std::tie(net, dev) = vent->findDevice(*it);
                    if (!dev) { 
                        this->write(createJsonRpc("DELETE", data, url + "/" + std::string(*it)));
                    }
                }
            }
            return;
        }

        if (exist(aj, "result")) {
            vent->logger()->info(" <--- Bastard response " + aj.dump());
            return;
        }
        else if (exist(aj, "error")) {
            vent->logger()->warn(" <--- Bastard error " + aj.dump());
            return;
        }
        //
        std::string method = aj.at("method");
        
        vent->logger()->info(" <--- Bastard request {}", aj.dump());

        if((method == "POST" || method == "PUT") && !updateItem(aj, false)) {
            vent->logger()->warn("Can not update item " + aj.dump());
            this->write(errorMsg(rpcid, "can not update item"));
            return;
        }
        else if (method == "DELETE" && !deleteItem(aj, false)) {
            vent->logger()->warn("Can not delete item " + aj.dump());
            this->write(errorMsg(rpcid, "can not delete item"));
            return;
        }

        if (method != "POST" && method != "PUT" && method != "DELETE") {
            vent->logger()->warn("not implemented.. " + aj.dump());
            this->write(errorMsg(rpcid, "not implemented!"));
            return;
        }
        // positive response.
        this->write(responseMsg(rpcid));
    }

    void BackendSocket::readCallback(ev::io&, int)
    {
        bastardReady_  = true;
        Ventilator* vent = Ventilator::instance();

        std::string data = sslclient_.read(); 
        if (data.empty()) {
            vent->logger()->warn("Nothing to read - returning.");
            return;
		}
        
        json ajsons;
        try {
            if (!chunk_.empty())
                ajsons = json::parse(chunk_ + data);
            else 
                ajsons = json::parse(data);
        }
        catch(json::parse_error& e) {
            vent->logger()->warn("Json data incomplete - waiting for next package."); 
            chunk_ += data;
            return;
        }
        // clear. 
        if (!chunk_.empty())
            chunk_.clear();

        try {
            if (ajsons.is_array()) {
                for (const auto& aj : ajsons) 
                    parseJson(aj);
            }
            else if (ajsons.is_object()) 
                parseJson(ajsons);
        }
        catch(json::type_error& e) {
            vent->logger()->error("-------------------");
            vent->logger()->error(data);
            vent->logger()->error(e.what());
        }
        catch(std::exception& e) {
            vent->logger()->error("-------------------");
            vent->logger()->error(data);
            vent->logger()->error(e.what());
        }
        
        // check if there is something to send, if so send to Bastard.
        trySend();
    }
    
    std::string BackendSocket::read() const
    {
        std::string data = sslclient_.read(); 

        bastardReady_ = true;
        if (data.empty()) {
            Ventilator::instance()->logger()->warn("Nothing to read - returning.");
            return "";
		}
    
        return data;
    }

    void BackendSocket::write(const json& ajson) const 
    {
        Ventilator* vent = Ventilator::instance();

        if (!bastardReady_) {
            if (messages_.size() > MSG_LIMIT) {
                vent->logger()->warn("Limit of messages has been reached! Droping message.");
                return;
            }
            messages_.push_back(ajson);
            return;
        }
        
        // bastardReady = false, only if it is not response.
        if (!exist(ajson, "result")) {
            bastardReady_ = false;
        }

        if (messages_.empty()) {
            sslclient_.write(ajson.dump());
            vent->logger()->info(ajson.dump());
        }
        else {
            messages_.push_back(ajson);
            json ajs(messages_);
            sslclient_.write(ajs.dump());

            messages_.clear();
            vent->logger()->info(ajs.dump());
        }
    }
    
    void BackendSocket::trySend() const
    {
        if (!bastardReady_ || messages_.empty()) 
            return;

        Ventilator* vent = Ventilator::instance();
        json ajs(messages_);
        int bytes = sslclient_.write(ajs.dump());
        if (bytes == 0) {
            vent->logger()->warn("Connection with backend has been lost. "); 
            wconnect_.start();
        }

        messages_.clear();
        vent->logger()->info(ajs.dump());
    }

    bool BackendSocket::isBackend() const
    {
        return true;
    }

} // namespace
