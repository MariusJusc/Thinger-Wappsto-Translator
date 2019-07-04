#pragma once

#include "configuration.hpp"
#include "localsocket.hpp"
#include <ev++.h>
#include "client.hpp"
#include <tuple>

#include "spdlog/spdlog.h"
#include "spdlog/async_logger.h"
#include "json.hpp"

using json = nlohmann::json;

namespace qplus {

    class Root;
    class Network;
    class Device;
    class Value;
    class State;
    class BackendSocket;

    class Ventilator
    {
        public:
            static Ventilator* instance();

            //void allToClient(const std::string& uuid) const;
            void allToBackend() const;
            void getBastardDeviceList() const; 

            ev::default_loop& defaultLoop();

            Conf_t conf() const;
            std::unique_ptr<Root> root;

            const Client* findClient(const std::string& uuid) const;
            Network* findNetwork(const std::string& uuid) const;
            std::tuple<Network*, Device*> findDevice(const std::string& uuid) const;
            std::tuple<Device*, Value*> findValue(const std::string& uuid) const;
            std::tuple<Value*, State*> findState(const std::string& uuid) const;

            const BackendSocket* bSocket() const;
            const LocalSocket* lSocket() const;
            void clean();
            void registerNetwork() const;
            void connect();
            void cleanClients();

            std::shared_ptr<spdlog::async_logger> logger();

        private:
            Ventilator();
            ~Ventilator() = default;
            Ventilator(const Ventilator&) = delete;
            Ventilator& operator=(const Ventilator&) = delete;

            void acceptCallback(ev::io& watcher, int revent);
            void cleaningDevices(ev::timer& watcher, int revent);
            void cleaningClients(ev::timer& watcher, int revent);

        private:
            Conf_t conf_;
            std::vector<std::unique_ptr<Client>> clients;

            mutable std::vector<json> messages;
            mutable bool bastardReady_ = true;

            std::shared_ptr<spdlog::async_logger> logger_ = nullptr;

            std::unique_ptr<BackendSocket> backendSocket_;
            std::unique_ptr<LocalSocket> localSocket_;

            ev::default_loop loop;
            ev::io waccept;
            ev::io lio;

            ev::timer wcleanDevices_;
            ev::timer wcleanClients_;
    };

} // namespace qplus
