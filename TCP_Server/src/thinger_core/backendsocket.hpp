#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

class backendsocket {

public:
    void connect()
    {
        std::string ipAdress = "10.0.2.15";
        int port = 21004;

        //Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockfd == -1) {
            perror("[ERROR]: Failed to create backend socket: ");
            exit(0);
        }

        // Fill out hint structure
        struct sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ipAdress.c_str(), &hint.sin_addr);

        // Connect to backend
        int connection = ::connect(sockfd, (sockaddr*)& hint, sizeof(hint));
        if(connection == -1) {
            perror("[ERROR]: Failed to connect to backend server: ");
            exit(0);
        }

        char buff[4096];
        std::string userInput;

        std::string message = "hello world!"; // Test message for debugging
        ssize_t written = ::write(sockfd, message.c_str(), message.size() + 1);
    }
};