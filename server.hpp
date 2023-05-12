#ifndef SERVER_HPP
#define SERVER_HPP
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <poll.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include <map>
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

#define BUFFER_SIZE 1024

class Client;
class Server
{
    private:
        int                     _fd;
        int                     _port;
        bool                    _running;
        std::string             _password;
        std::vector<pollfd>     _fdsVector;
        // std::vector<Channel>    _channels;
        std::map<int, Client*>  _clients;
        void  parseCommand(Client *client, std::string &command);
        void  processCommand(Client *client, std::vector<std::string> tokens);
    
    public:
        Server();
        Server(int port, const std::string &password);
        ~Server();
        int getFd() const;
        int getPort() const;
        std::string getPassword() const;
        bool isRunning() const;
        void    InitSocket();
        bool    readFromClient(Client *client);
        void    writeToClient();
        void    removeClient(int client_fd);
        std::string normalizeLineEnding(std::string &str);
        void sendMessage(int fd, std::string message);
        void _nickCommand(Client *client, std::vector<std::string> tokens);
        std::string    readFromClient(int client_fd);
		bool	authenticateUser(int client_fd);

        // Exceptions 
        class InvalidSocketFd : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
        class FcntlError : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
        class SetsockoptError : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
        class BindError : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
        class ListenError : public std::exception
        {
            public:
                virtual const char *what() const throw();
        };
};

#endif