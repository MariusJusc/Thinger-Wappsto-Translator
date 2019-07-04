#pragma once

#include <string>
#include <arpa/inet.h>
#include "configuration.hpp"

namespace qplus {

    class LocalSocket
    {
        public:
            explicit LocalSocket(const Conf_t& conf);
            int descriptor() const;
            bool backend() const;
            
        private:
            //void accept_cb(ev::io& wio, int revent);

        private:
            int fdescriptor_;
            struct sockaddr_in addr_;
    };

} // namespace
