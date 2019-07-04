#pragma once

#include <arpa/inet.h>
#include <ev++.h>
#include <string>
#include <list>
#include "json.hpp"

using json = nlohmann::json;

namespace qplus {

    struct JsonParams
    {
        std::string data;   
        std::string url;
    };

    struct JsonFormat 
    {
        std::string id;
        std::string jsonrpc;
        std::string method;
        JsonParams params;
    };

    class Client 
    {
        public:
            Client(int fd);
            ~Client();

            bool active() const;
            std::string uuid() const;
            
            void checkUpdates() const;
            void write(const json& ajson) const;  // request
            // For generic function check.
            bool isBackend() const;

        private:
            void readCallback(ev::io& w, int);
            std::string peer_address(struct sockaddr* addr);
            void sendResponse(const json& ajson) const;
            
            void receiveTimeout(ev::timer& w, int revent);
            void deserialize(const std::string& uuid) const;
        private:
            ev::io wclient_;
            struct sockaddr_in client_addr_;
            int fdc_;
            bool active_ = false;
            mutable bool clientReady_ = false;

            std::string uuid_;
            mutable std::list<json> messages_;
            
            std::string rcvBuffer_;
            // read timer - if message is truncated, allow next message withing specified interval. After - clean
            // message buffer: rcvBuffer_
            ev::timer wread_;
    };

} // namespace qplus
