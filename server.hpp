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
// #include <bits/stdc++.h>
#include <map>
#include <arpa/inet.h>
#include "Channel.hpp"
#include "errorcode.hpp"
#include "rplcode.hpp"
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

#define BROADCAST 0
#define UNICAST 1

#define BUFFER_SIZE 1024

class Client;
class Channel;
class Server
{
    private:
        int                     _fd;
        int                     _port;
		std::string				creationDate;
		std::string				_serverName;
        bool                    _running;
        std::string             _password;
        std::vector<pollfd>     _fdsVector;
        std::vector<Channel*>    _channels;
        std::map<int, Client*>  _clients;
		std::map<std::string, Client*> _nicknames;
		std::map<int, std::string> errCodeToStr;
		std::map<int, std::string> rplCodeToStr;
        void	parseCommand(Client *client, std::string &command);
        void	processCommand(Client *client, std::vector<std::string> tokens);
        void	handleClient(Client *client);
		bool	nickAvailable(std::string nick);
		std::string	getDate(void);
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
        void sendMessage(Client *src, Client *dst, int ERRCODE, int RPLCODE ,std::string message);
        void _nickCommand(Client *client, std::vector<std::string> tokens);
		void _userCommand(Client *client, std::vector<std::string> tokens);
		void _passCommand(Client *client, std::vector<std::string> tokens);
        void _joinCommand(Client *client, std::vector<std::string> tokens);
        void _listCommand(Client *client, std::vector<std::string> tokens);
		void	_privMsgCommand(Client *client, std::vector<std::string> tokens);
        std::vector<std::string> SplitTargets(std::vector<std::string> tokens);
        std::string    readFromClient(int client_fd);
        void    CheckAuthentication(Client *client);
        bool    nickAvailable(std::string nickname) const;
        Channel *_findChannel(std::string channelName) const;
		char	*getAddr(Client *clt);
		void	checkAndAuth(Client *clt);
		void	initCode();
        void    BroadcastMessage(Client *client, Channel *target,const std::string &message);
		void    findTargetsAndSendMessage(Client *client, std::vector<std::string> recipients, std::string message, std::string command);
        void    SendToRecipients(Client *client, std::vector<std::string> recipients, std::string message, std::string command);
		void    SendToRecipient(Client *client, std::vector<std::string> recipients, std::string message, bool isChannel, std::string command);
        void    AddMember(Client *client, Channel *channel);
        std::vector<std::string> CheckAndSeparate(Client *client, std::vector<std::string> tokens);
        friend class Channel;
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