#include "client.hpp"

#include "json.hpp"
#include <unistd.h>
#include <iostream>
#include "ventilator.hpp"
#include "root.hpp"
#include "network.hpp"
#include "dataup.hpp"
#include "util.hpp"
#include <list>

using json = nlohmann::json;

namespace qplus {

    Client::Client(int fd)
    {
        // Accept client request
        socklen_t client_len = sizeof(client_addr_);

        fdc_ = accept(fd, (struct sockaddr *)&client_addr_, &client_len);
        if (fdc_ <  0) {
            perror("accept error");
            return;
        }

        std::string peerAddress = peer_address((struct sockaddr*)&client_addr_);
        int peerPort = client_addr_.sin_port;
        printf("Connected peer: %s, port %d\n", peerAddress.c_str(), peerPort);

        // --------------------------------
        wclient_.set<Client, &Client::readCallback>(this);
        wclient_.set(fdc_, ev::READ);
        wclient_.start();
        
        wread_.set(0., 2._s); 
        wread_.set<Client, &Client::receiveTimeout>(this);

        active_ = true;
    }

    Client::~Client() {
        close(fdc_);
    }
    
    void Client::receiveTimeout(ev::timer& w, int revent)
    {
        w.stop();
        Ventilator::instance()->logger()->info("Receive timeout {}", revent);
        
        // Buffer should be empty if parsing json has been succeeded. 
        if (!rcvBuffer_.empty()) {
            sendResponse(errorMsg("0666", "Json parsing error!"));
            rcvBuffer_.clear();
        }
    }
    
    /**
     * 
     */
    void Client::readCallback(ev::io& watcher, int revents)
    {
        Ventilator* vent = Ventilator::instance();
        if (EV_ERROR & revents) {
            vent->logger()->info(" ---> client callback. Invalid event");
            return;
        }
        
        if (fdc_ < 0 || !active_)
            vent->cleanClients();

        unsigned char data[4096];
        memset(data, 0, 4096);
        ssize_t n = recv(fdc_, data, 4096, 0);
        if (n < 0) {
            vent->logger()->info(" ---> client callback. Read ERROR");
            return;
        }
        else if (n == 0) {
            // Stop and free watchet if client socket is closing
            watcher.stop();
            vent->logger()->info(" ---> client callback. Client CLOSING");
            active_ = false;
            vent->cleanClients();
            return;
        }
 
        vent->logger()->info(" ---> client callback. Client READY");
        clientReady_ = true;
       
        // Parse and update items. First client should send its network uuid for registration.
        std::string rpcid;
        try {
            rcvBuffer_ += std::string(data, data + n);
            auto messages = findJsons(rcvBuffer_);
            // once parsing succeeded, clear buffer for next (big) message.
            rcvBuffer_.clear();

            for (const auto& msg : messages) { 
                json aj = json::parse(msg);
                rpcid = aj["id"];
                // 
                if (exist(aj, "result")) {
                    vent->logger()->info(" ---> Client response " + aj.dump());
                    continue; 
                }

                // Network
                if (uuid_.empty()) {
                    if (split(aj["/params/url"_json_pointer]).back() != "network") {
                        sendResponse(errorMsg(rpcid, "Send network frame first."));
                        continue;
                    }

                    // --
                    uuid_ = aj["/params/data/:id"_json_pointer];
                    if (uuid_.empty()) {
                        sendResponse(errorMsg(rpcid, "Network uuid is empty."));
                        continue;
                    }

                    sendResponse(responseMsg(rpcid));
                    
                    vent->logger()->info("Registration of network: {}", uuid_);

                    Ventilator* vent = Ventilator::instance();
                    Network* net = vent->findNetwork(uuid_);
                    if (net) {
                        // Send all serialized data if exist to that client.
                        vent->logger()->info("Sending serialized data to client: {}", uuid_);
                        deserialize(uuid_);
                    }
                    else {
                        // Save network to the list.
                        vent->logger()->info("Saving client: {} to the network list.", uuid_);
                        std::unique_ptr<Network> net = std::unique_ptr<Network>(new Network(vent->root.get(), uuid_));
                        vent->root->networks.push_back(std::move(net));
                    }
                    continue;
                }
                else if(isPost(aj) && !updateItem(aj, true)) {
                    sendResponse(errorMsg(rpcid, "Can not update item"));
                    continue;
                }
                else if (isPut(aj) && !updateItem(aj, true)) {
                    sendResponse(errorMsg(rpcid, "Can not update item"));
                    continue;
                }
                else if (isDelete(aj) && !deleteItem(aj, true)) {
                    sendResponse(errorMsg(rpcid, "Can not delete item"));
                    continue;
                }
                else if (!isPost(aj) && !isPut(aj) && !isDelete(aj)) {
                    sendResponse(errorMsg(rpcid, "Not implemented."));
                    continue;
                }
                else { 
                    // Positive response.
                    sendResponse(responseMsg(rpcid));
                }
            } // for messages
        }
        catch(json::parse_error& e) {
            if (rcvBuffer_.empty()) {
                wread_.start();
            }
            vent->logger()->warn("Incomplete json, waiting for next message...");
            return;
        }
        catch(json::type_error& e) {
            if (rpcid.empty())
                sendResponse(errorMsg("0555", e.what()));
            else
                sendResponse(errorMsg(rpcid, e.what()));
        }
        catch(std::exception& e) {
            sendResponse(errorMsg("0777", e.what()));
        }
        // 
        checkUpdates();
    }
    
    /**
     * Provate method only called by client request: Client::readCallback
     */
    void Client::sendResponse(const json& ajson) const
    {
        Ventilator* vent = Ventilator::instance();
        if (fdc_ < 0) {
            vent->logger()->error("Socket is not connected, can NOT send data"); 
            return;
        }
        
        std::string msg = ajson.dump();
        ::send(fdc_, msg.data(), msg.size(), 0);         
    }

    // Method is only used to update client - for client response 'sendResponse' method is used.
    void Client::write(const json& ajson) const
    {
        Ventilator* vent = Ventilator::instance();
        if (fdc_ < 0) {
            std::cerr << "Socket is not connected, can NOT send data\n"; 
            return;
        }
        
        if (!clientReady_) {
            messages_.push_back(ajson);
            std::cout << "Client not ready\n";
            return;
        }

        // change state and send all messages to client 
        clientReady_ = false;
        vent->logger()->warn("Client BUSY");

        if (messages_.empty()) {
            std::string msg = ajson.dump();
            ssize_t sended = ::send(fdc_, msg.data(), msg.size(), 0);  
            std::cout << "----Sended single \n" << msg << "\n";
            vent->logger()->info(" <--- To client " + ajson.dump());
        }
        else {
            messages_.push_back(ajson);
            json ajs(messages_);
            std::string msg = ajs.dump();
            ssize_t sended = ::send(fdc_, msg.data(), msg.size(), 0);  
            std::cout << " ---- Sended batch\n" << msg << "\n";
            vent->logger()->info(" <--- To client " + ajs.dump());
        }

        messages_.clear();
    }

    void Client::checkUpdates() const
    {
        if (!clientReady_) 
            return;
        
        Ventilator* vent = Ventilator::instance();

        // loop through all client devices, values and states, search for updated items - 
        // create jsons in the list - push to the client.
        // remove device, etc. is implemented directly creating json and calling client write method 
        // only  update.
        Network* network = vent->findNetwork(this->uuid());
        for (const auto& device : network->devices) {
            if (device->updated) {
                vent->logger()->info("Device " + device->desc.name + " needs to be updated for client.");
                device->updated = false; // reset
                json ajson;
                to_json(ajson, device->desc);       
                json payload = createJsonRpc("PUT", ajson, device->location() + device->desc.id);
                messages_.push_back(payload);
            }
            for (const auto& value : device->values) {
                for (const auto& state : value->states) {
                    if (state->updated) {
                        vent->logger()->info("State " + value->desc.name + " needs to be updated for client.");
                        state->updated = false; //reset
                        json ajson;
                        to_json(ajson, state->desc);       
                        json payload = createJsonRpc("PUT", ajson, state->location() + state->desc.id);
                        messages_.push_back(payload);
                    }
                } // state
            } // value
        } // device
        
        if (messages_.empty()) {
            std::cout << "Nothing to send\n";
            return;
        }
        
        clientReady_ = false;
        vent->logger()->warn("Client BUSY");

        // Client should accept a list of messages.
        json msg_array = json(messages_);
        messages_.clear();

        std::string msg = msg_array.dump();
        std::cout << " +++++++++++++++++++++++++++++++  Check updates: \n" << msg << "\n"; 

        ::send(fdc_, msg.data(), msg.size(), 0);  
        vent->logger()->info(msg);
    }
    
    void Client::deserialize(const std::string& uuid) const
    {
        Ventilator* vent = Ventilator::instance();
        
        vent->logger()->info("");
        vent->logger()->info("Deserializing client with uuid: " + uuid);

        std::string method{"POST"};
        Network* network = vent->findNetwork(uuid);
        if (network) {
            for (const auto& device : network->devices) {
                if (!device->box.empty()) {
                    write(createJsonRpc(method, device->box, getUrl(device->location(), false)));
                }
                for (const auto& value : device->values) {
                    if (!value->box.empty()) {
                        write(createJsonRpc(method, value->box, getUrl(value->location(), false)));
                    }
                    for (const auto& state : value->states) {
                        if (!state->box.empty()) {
                            write(createJsonRpc(method, state->box, getUrl(state->location(), false)));
                        }
                    } // state
                } // value
            } // device 
        } // network
    }

    /*
     *std::string Client::read() const
     *{
     *    unsigned char data[4096];
     *    memset(data, 0, 4096);
     *    ssize_t n = recv(fdc_, data, 4096, 0);
     *    
     *    std::cout << "Client read " << data << "\n";
     *    return std::string(data, data + n);
     *}
     */

    std::string Client::peer_address(struct sockaddr* addr)
    {
        char ipstr[INET_ADDRSTRLEN];
        struct sockaddr_in* s = (sockaddr_in*) addr;
        inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof(ipstr));
        return ipstr;
    }
        
    bool Client::active() const
    {
        return active_;
    }

    std::string Client::uuid() const
    {
        return uuid_;
    }

    bool Client::isBackend() const
    {
        return false;
    }
} // namespace 

