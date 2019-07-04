#pragma once

#include "json.hpp"
#include <vector>
#include <ev++.h>
#include "sslclient.hpp"
#include "configuration.hpp"

using json = nlohmann::json;

namespace qplus {

    class BackendSocket
    {
        public:
            explicit BackendSocket(const Conf_t& conf);
            bool connect();
            
            void write(const json& ajson) const;
            std::string read() const;
            bool isBackend() const;

        private:
            void readCallback(ev::io& w, int revents);
            void trySend() const;
            void parseJson(const json& aj);
            void reconnect(ev::timer& watcher, int revent);

        private:
            SSLClient sslclient_;
            std::string endpoint_;
            mutable bool bastardReady_ = true;
            mutable std::vector<json> messages_;
            
            ev::io bio;
            std::string chunk_;
            mutable ev::timer wconnect_;
    };


} // namespace
