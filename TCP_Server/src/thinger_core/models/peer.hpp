#include <iostream>
#include <vector>
#include <algorithm>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "thinger_credentials.hpp"
#include "thinger_stream.hpp"

#ifndef CLIENT_PEER_HPP
#define CLIENT_PEER_HPP

class peer
{

    private:
        int socket;
        struct sockaddr_in address;
        bool is_auth;


        std::vector<thinger_stream> t_stream;
        thinger_credentials credentials;

    public:
        peer() : socket(-1), is_auth(false) {}
        ~peer() {}

        bool create_peer(peer* p)
        {
            return true;
        }

        bool close_peer()
        {
            close(socket);

            socket = -1;
            is_auth = false;
            return true;
        }

        // GETTERS & SETTERS
        auto get_socket()       -> int& { return socket; }
        auto get_socket() const -> const int& { return socket; }

        auto get_address()       -> sockaddr_in& { return address; }
        auto get_address() const -> const sockaddr_in& { return address; }

        auto get_t_stream()       -> std::vector<thinger_stream>& { return t_stream; }
        auto get_t_stream() const -> const std::vector<thinger_stream>& { return t_stream; }

        auto get_is_auth()       -> bool& { return is_auth; }
        auto get_is_auth() const -> const bool& { return is_auth; }

        auto get_credentials()       -> thinger_credentials& { return credentials; }
        auto get_credentials() const -> const thinger_credentials& { return credentials; }

        void insert_stream(thinger_stream stream)
        {
            t_stream.push_back(stream);
        }

        thinger_stream* get_stream(uint16_t id)
        {
            for(auto &s : t_stream) 
                if(s.get_stream_id() == id)
                    return &s;

            return nullptr;
        }

        bool stream_exist(uint16_t id)
        {
            for(auto &s : t_stream)
                if(s.get_stream_id() == id)
                    return true;
            
            return false;
        }

        void remove_stream()
        {
            
        }
};

#endif