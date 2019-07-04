#include "localsocket.hpp"

#include <sys/socket.h>
#include <strings.h>

namespace qplus {

    LocalSocket::LocalSocket(const Conf_t& conf)
    {
		//int PORT_NO = 21004;
		fdescriptor_ = socket(AF_INET, SOCK_STREAM, 0);

        int option = 1; 
        setsockopt(fdescriptor_, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

		bzero(&addr_, sizeof(addr_));
		addr_.sin_family = AF_INET;
		addr_.sin_port = htons(conf.LISTENING_PORT);
		addr_.sin_addr.s_addr = INADDR_ANY;

		::bind(fdescriptor_, (struct sockaddr*) &addr_, sizeof(addr_));        
		::listen(fdescriptor_, 16);
    }

	int LocalSocket::descriptor() const
	{
		return fdescriptor_;
	}

} // namespace qplus
