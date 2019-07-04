#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "thinger_controller.h"
#include "models/peer.hpp"

using namespace protoson;

dynamic_memory_allocator alloc;
memory_allocator &protoson::pool = alloc;

class thinger_server : public thinger::thinger_controller
{

private:
    int sockfd, max_clients = 10;

    uint8_t *out_buffer_;
    size_t out_size_;
    size_t buffer_size_;

    int listen_socket;
    peer connection_list[10];

public:
    thinger_server() : sockfd(-1), out_buffer_(NULL), out_size_(0), buffer_size_(0)
    {
    }

    void start()
    {
        start_server();
    }

protected:
    virtual bool read(char *buffer, size_t size)
    {
        if (sockfd == -1)
            return false;

        ssize_t read_size = ::read(sockfd, buffer, size);

        if (read_size != size)
        {
            std::cout << "Line - 45 Disconnect..." << std::endl;
            //disconnected();
        }

        return read_size == size;
    }

    virtual bool write(const char *buffer, size_t size, bool flush = false)
    {
        if (size > 0)
        {
            if (size + out_size_ > buffer_size_)
            {
                buffer_size_ = out_size_ + size;

                // Changes the size of the memory block
                out_buffer_ = (uint8_t *)realloc(out_buffer_, buffer_size_);
            }

            // Copy block of memory
            memcpy(&out_buffer_[out_size_], buffer, size);
            out_size_ += size;
        }

        if (flush && out_size_ > 0)
        {
            bool success = to_socket(out_buffer_, out_size_);

            out_size_ = 0;

            if (!success)
            {
                std::cout << "[157]: Disconnect..." << std::endl;
                //disconnected();
            }
            return success;
        }
        return true;
    }

private:
    bool handle_client(peer &p)
    {
        bool handle = thinger::thinger_controller::handle_input(p);

        if (handle == true)
            return true;


        std::cout << "Message handle fail..." << std::endl;
        return false;
    }

    bool to_socket(const uint8_t *buffer, size_t size)
    {
        if (sockfd == -1)
            return false;

        ssize_t written = ::write(sockfd, buffer, size);

        return out_size_ == written;
    }

    bool start_listen_socket(int *listen_sock)
    {
        // Create a master socket
        *listen_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (listen_sock < 0)
        {
            perror("[ERROR]: Failed to open socket: ");
            return false;
        }

        int reuse = 1;
        if (setsockopt(*listen_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) != 0)
        {
            perror("[ERROR]: Setsockopt ");
            return false;
        }

        // Configure IP and Ser(ver ports
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(50000);

        // Bind host address
        if (bind(*listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
        {
            perror("[ERROR]: Failed to bind the configurations: ");
            return false;
        }

        // Start listening incoming connections
        if (listen(*listen_sock, 10) != 0)
        {
            perror("[SERVER]: Failed to start listening: ");
            return false;
        }

        std::cout << "Server started listening on port " << serv_addr.sin_port << std::endl;

        return true;
    }

    bool build_fd_sets(fd_set *read_fds, fd_set *write_fds, fd_set *except_fds)
    {
        // read file descriptors
        FD_ZERO(read_fds);
        FD_SET(listen_socket, read_fds);
        for (int i = 0; i < max_clients; i++)
        {
            /*if (connection_list[i].socket != -1)
            {
                FD_SET(connection_list[i].socket, read_fds);
            }*/
            if (connection_list[i].get_socket() != -1)
            {
                FD_SET(connection_list[i].get_socket(), read_fds);
            }
        }

        // Write file descriptors
        FD_ZERO(write_fds);
        for (int i = 0; i < max_clients; ++i)
        {
            if (connection_list[i].get_socket() != -1 && out_buffer_ > 0)
            {
                FD_SET(connection_list[i].get_socket(), write_fds);
            }
        }

        // Except file descriptors
        FD_ZERO(except_fds);
        FD_SET(listen_socket, except_fds);

        for (int i = 0; i < max_clients; ++i)
        {
            if (connection_list[i].get_socket() != -1)
            {
                FD_SET(connection_list[i].get_socket(), except_fds);
            }
        }

        return true;
    }

    bool handle_new_connection()
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);

        int new_client_sock = accept(listen_socket, (struct sockaddr *)&client_addr, &client_len);
        if (new_client_sock < 0)
        {
            perror("[SERVER]: Error in accepting a new client: ");
            return false;
        }

        char client_ipv4_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ipv4_str, INET_ADDRSTRLEN);

        std::cout << "[SERVER]: Incoming connection from " << client_ipv4_str << ":" << client_addr.sin_port << std::endl;

        for (int i = 0; i < max_clients; i++)
        {
            if (connection_list[i].get_socket() == -1)
            {
                connection_list[i].get_socket() = new_client_sock;
                connection_list[i].get_address() = client_addr;
                connection_list[i].get_is_auth() = false;
                return true;
            }
        }

        std::cout << "[SERVER]: There are too much connections at the moment. Closing new connection " << client_ipv4_str << client_addr.sin_port << std::endl;
        close(new_client_sock);

        return false;
    }

    void close_client_connection(peer &client)
    {
        // TODO: Fully cleanup disconnected peer!
        std::cout << "[SERVER]: Close peer connection." << std::endl;
        client.close_peer();
    }

    void shutdown(int code)
    {
        close(listen_socket);

        for (int i = 0; i < max_clients; i++)
        {
            if (connection_list[i].get_socket() != -1)
                close(connection_list[i].get_socket());
        }

        std::cout << "Server is shutting down!" << std::endl;
        exit(code);
    }

    bool start_server()
    {
        if (start_listen_socket(&listen_socket))
        {
            fd_set read_fds;
            fd_set write_fds;
            fd_set except_fds;

            int high_sock = listen_socket;

            // Notify incoming connections.
            std::cout << "[SERVER]: Waiting for incoming connections." << std::endl;

            while (true)
            {
                build_fd_sets(&read_fds, &write_fds, &except_fds);

                high_sock = listen_socket;

                for (int i = 0; i < max_clients; ++i)
                {
                    if (connection_list[i].get_socket() > high_sock)
                        high_sock = connection_list[i].get_socket();
                }

                int activity = select(high_sock + 1, &read_fds, &write_fds, &except_fds, nullptr);

                switch (activity)
                {
                case -1:
                    perror("[SERVER]: Error in select: ");
                    shutdown(EXIT_FAILURE);
                case 0:
                    shutdown(EXIT_FAILURE);
                default:

                    if (FD_ISSET(listen_socket, &read_fds))
                    {
                        handle_new_connection();
                    }

                    if (FD_ISSET(listen_socket, &except_fds))
                    {
                        std::cerr << "Exception in listen socket fd." << std::endl;
                        shutdown(EXIT_FAILURE);
                    }

                    for (int i = 0; i < max_clients; ++i)
                    {
                        if(connection_list[i].get_socket() != -1 && FD_ISSET(connection_list[i].get_socket(), &read_fds))
                        {
                            sockfd = connection_list[i].get_socket();
                            if(!handle_client(connection_list[i]))
                            {
                                close_client_connection(connection_list[i]);
                                continue;
                            }
                        }

                        if (connection_list[i].get_socket() != -1 && FD_ISSET(connection_list[i].get_socket(), &write_fds))
                        {
                            sockfd = connection_list[i].get_socket();
                        }

                        if (connection_list[i].get_socket() != -1 && FD_ISSET(connection_list[i].get_socket(), &except_fds))
                        {
                            printf("Exception client fd.\n");
                            close_client_connection(connection_list[i]);
                            connection_list[i].close_peer();
                            continue;
                        }
                    }
                }
            }
        }
    }
};
