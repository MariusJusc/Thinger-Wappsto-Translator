#include "ventilator.hpp"

#include "root.hpp"
#include "util.hpp"
#include "path.hpp"
#include <string>
#include <cassert>
#include "dataup.hpp"
#include "serializer.hpp"
#include "backendsocket.hpp"

#include "spdlog/sinks/ansicolor_sink.h"
#include "spdlog/sinks/file_sinks.h"

namespace qplus {
    
    // Asynchronous logger with multi sinks
    std::shared_ptr<spdlog::async_logger> initLogger(const Conf_t& conf) 
    {
        auto stdout_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(conf.QPLUS_PATH + "/logs/qplus.log", 1024*1024*10, 3);

        std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
        int queue_size = 1024;
        auto logger = std::make_shared<spdlog::async_logger>("log", sinks.begin(), sinks.end(), queue_size);

        //logger->set_level(spdlog::level::info); // Set global log level to info - debug() will not be desplayed.
        logger->set_level(spdlog::level::trace); // Set specific logger's log level

        spdlog::register_logger(logger);
        return logger;
    }

    Ventilator* Ventilator::instance()
    {
        static Ventilator instance;
        return &instance;
    }

    Ventilator::Ventilator() : conf_(readConfiguration())
    {
        // -- Create Network 
        Path path(conf_.QPLUS_PATH);
        assert(path.directory() != path.dirs_end());
        root = std::unique_ptr<Root>(new Root(conf_));
    
        localSocket_ = std::unique_ptr<LocalSocket>(new LocalSocket(conf_));

        // Initialize and start a watcher to accepts client requests
		waccept.set<Ventilator, &Ventilator::acceptCallback>(this);
		waccept.set(localSocket_->descriptor(), ev::READ);
		waccept.start();
    
        wcleanDevices_.set(3._min, 3._min); 
        wcleanDevices_.set<Ventilator, &Ventilator::cleaningDevices>(this);
        wcleanDevices_.start();
    
        wcleanClients_.set(0.1_s, 0.0); 
        wcleanClients_.set<Ventilator, &Ventilator::cleaningClients>(this);

        try {
            logger_ = initLogger(conf_);
            logger_->info("Logger successfully initialized!");
        }
        catch(std::exception& e) {
            std::string err = "Failed to initialize logger! Check " + conf_.QPLUS_PATH + "/logs/qplus.log access permissions.";            
            throw error_t(err);
        }

        backendSocket_ = std::unique_ptr<BackendSocket>(new BackendSocket(conf_));
    }
    
    void Ventilator::connect()
    {
        backendSocket_->connect();
    }

    std::shared_ptr<spdlog::async_logger> Ventilator::logger()
    {
        return logger_;
    }
    
    void Ventilator::getBastardDeviceList() const
    {
        json data{};
        std::string url = "/network/" + conf_.QW_UUID + "/device";
        this->bSocket()->write(createJsonRpc("GET", data, url));
    }

    /*
     *void Ventilator::allToClient(const std::string& uuid) const
     *{
     *    std::cout << "\n";
     *    std::cout << "------------- Sending all data to Client "  + uuid + " -----------------\n";
     *    std::cout << "\n";
     *    
     *    auto client = std::find_if(clients.begin(), clients.end(),
     *                               [&uuid](const std::unique_ptr<Client>& cl)
     *                               {
     *                                    return cl->uuid() == uuid;
     *                               });
     *
     *    Client* cl = (*client).get(); 
     *    
     *    std::string method{"POST"};
     *    Network* network = findNetwork(uuid);
     *    if (network) {
     *        for (const auto& device : network->devices) {
     *            device->updated = true;
     *            //cl->write(createJsonRpc(method, device->box, getUrl(device->location(), false)));
     *            for (const auto& value : device->values) {
     *                value->updated = true;
     *                //cl->write(createJsonRpc(method, value->box, getUrl(value->location(), false)));
     *                for (const auto& state : value->states) {
     *                    state->updated = true;
     *                    //cl->write(createJsonRpc(method, state->box, getUrl(state->location(), false)));
     *                } // state
     *            } // value
     *        } // device 
     *    }
     *    else {
     *        logger_->critical("Can not find network with uuid: " + uuid);
     *    }
     *    
     *    cl->trySend(); // send all info to client.
     *}
     */
    
    void Ventilator::clean() 
    {
        std::cout << "Cleaning ----------------------- \n";
        for (const auto& network : root->networks) {
            auto it = std::begin(network->devices);
            while (it != std::end(network->devices))
            {
                if ((*it)->desc.included == "0") {
                    post("DELETE", this->bSocket(), (*it).get());
                    removeDir((*it)->location() + (*it)->desc.id);
                    it = network->devices.erase(it);
                }
                else { ++it; }
            }
        }
    }
    
    void Ventilator::registerNetwork() const
    {
        std::cout << "\n";
        std::cout << "------------- Network registration to IoT server -----------------\n";
        std::cout << "\n";
        // Send network.
        post("POST", this->bSocket(), root.get()); 
    }

    void Ventilator::allToBackend() const
    {
        std::string method{"POST"};
        // 
        for (const auto& network : root->networks) {
            for (const auto& device : network->devices) {
                post(method, this->bSocket(), device.get());
                for (const auto& value : device->values) {
                    post(method, this->bSocket(), value.get());
                    for (const auto& state : value->states) {
                        post(method, this->bSocket(), state.get());
                    }
                }
            }
        }
    }
    
    Conf_t Ventilator::conf() const 
    {
        return conf_;
    }

	void Ventilator::acceptCallback(ev::io& watcher, int revent)
	{
		if(EV_ERROR & revent)
		{
			perror("got invalid event");
			return;
		}

        // Before creating a new Client, erase not active clients 
        cleanClients();

        auto client = std::unique_ptr<Client>(new Client(watcher.fd));
        clients.push_back(std::move(client));
        
        std::cout << "Connected clients " << clients.size() << "\n";
	}
    
    void Ventilator::cleaningClients(ev::timer&, int) 
    {
        auto it = clients.begin();
        while( it != clients.end() ) {
            if (!(*it)->active()) {
                logger_->warn("Removing non active client " + (*it)->uuid() + " from the list.");
                it = clients.erase(it);
            }
            else {
                ++it;
            }
        }

        wcleanClients_.stop();
    }

    void Ventilator::cleanClients()
    {
        wcleanClients_.start();
    }

    void Ventilator::cleaningDevices(ev::timer& watcher, int)
    {
        logger_->info("Check for un-included devices. Next check after [{}] sec.", watcher.remaining());
        clean();
    }

    Network* Ventilator::findNetwork(const std::string& uuid) const
    {
        auto results = std::find_if(root->networks.begin(),
                                    root->networks.end(),
                                    [&uuid](const std::unique_ptr<Network>& network)
                                    {
                                        return network->desc.id == uuid;
                                    });

        if (results != root->networks.end())
            return results->get();
        else
            return nullptr;
    }
    
    const Client* Ventilator::findClient(const std::string& uuid) const
    {
         auto results = std::find_if(clients.begin(),
                                    clients.end(),
                                    [&uuid](const std::unique_ptr<Client>& client)
                                    {
                                        return client->uuid() == uuid;
                                    });

        if (results != clients.end())
            return results->get();
        else
            return nullptr;
    }

    std::tuple<Network*, Device*> Ventilator::findDevice(const std::string& uuid) const
    {
        Device* results = nullptr;
        Network* parent = nullptr;

        for (auto& net : root->networks) {
            for (auto& dev : net->devices) {
                if (dev->desc.id == uuid) {
                    results = dev.get();
                    parent = net.get();
                    break;
                }
            }
        }
        return std::make_tuple(parent, results);
    }

    std::tuple<Device*, Value*> Ventilator::findValue(const std::string& uuid) const
    {
        Value* results = nullptr;
        Device* parent = nullptr;

        for (auto& net : root->networks) {
            for (auto& dev : net->devices) {
                for (auto& val : dev->values) {
                    if (val->desc.id == uuid) {
                        results = val.get();
                        parent = dev.get();
                        break;
                    }
                }
            }
        }
        return std::make_tuple(parent, results);
    }

    std::tuple<Value*, State*> Ventilator::findState(const std::string& uuid) const
    {
        State* results = nullptr;
        Value* parent = nullptr;

        for (auto& net : root->networks) {
            for (auto& dev : net->devices) {
                for (auto& val : dev->values) {
                    for (auto& st : val->states) {
                        if (st->desc.id == uuid) {
                            results = st.get();
                            parent = val.get();
                            break;
                        }
                    }
                }
            }
        }
        return std::make_tuple(parent, results);
    }

    const BackendSocket* Ventilator::bSocket() const
    {
        return backendSocket_.get();
    }

    const LocalSocket* Ventilator::lSocket() const
    {   
        return localSocket_.get();
    }

    ev::default_loop& Ventilator::defaultLoop()
    {
        return loop;
    }


} // namespace

int main()
{
    // Create and load all serialized data to Singleton instance.
    qplus::Ventilator* vent = qplus::Ventilator::instance(); 

    vent->connect();
    //
    vent->getBastardDeviceList();
    // Send data to Backend. 
    vent->allToBackend();
    // start a loop.
    vent->defaultLoop().run(0);

    return 0;
}
