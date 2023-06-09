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
#include <map>
#include <arpa/inet.h>
#include "../includes/Channel.hpp"
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
        int                     _nbrchannels;
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
        void	parseCommand(Client *client);
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
        std::string readFromClient(Client *client);
        void    writeToClient();
        void    removeClient(int client_fd);
        std::string normalizeLineEnding(std::string &str);
		void theBootLegSendMessage(Client *dst, std::string msg);
        void sendMessage(Client *src, Client *dst, int ERRCODE, int RPLCODE ,std::string message);
        void _nickCommand(Client *client, std::vector<std::string> tokens);
		void _userCommand(Client *client, std::vector<std::string> tokens);
		void _passCommand(Client *client, std::vector<std::string> tokens);
        void _joinCommand(Client *client, std::vector<std::string> tokens);
        void _listCommand(Client *client, std::vector<std::string> tokens);
		void _privMsgCommand(Client *client, std::vector<std::string> tokens);
        void _kickCommand(Client *client, std::vector<std::string> tokens);
        void _partCommand(Client *client, std::vector<std::string> tokens);
        void _topicCommand(Client *client, std::vector<std::string> tokens);
        void _inviteCommand(Client *client, std::vector<std::string> tokens);
        void _botCommand(Client *client, std::vector<std::string> tokens);
        void _pingCommand(Client *client, std::vector<std::string> tokens);
		void _modeCommand(Client *client, std::vector<std::string> tokens);
        Client *findClientByNickname(const std::string &nickname);
        std::vector<std::string> SplitTargets(std::string tokens);
        // std::string    readFromClient(int client_fd);
        void    CheckAuthentication(Client *client);
        bool    nickAvailable(std::string nickname) const;
        Channel *_findChannel(std::string channelName) const;
		char	*getAddr(Client *clt);
		void	checkAndAuth(Client *clt);
		void	initCode();
        void    DeleteEmptyChan(Channel* channel, std::vector<Channel*>& _channels);
        void    BroadcastMessage(Client *client, Channel *target,const std::string &message);
		void    findTargetsAndSendMessage(Client *client, std::vector<std::string> recipients, std::string message, std::string command);
        void    SendToRecipients(Client *client, std::vector<std::string> recipients, std::string message, std::string command);
		void    SendToRecipient(Client *client, std::vector<std::string> recipients, std::string message, bool isChannel, std::string command);
        void    AddMember(Client *client, Channel *channel);
        void    YeetMember(Client *oper, Client *target, Channel *channel, std::string reason);
		void	_quitCommand(Client *clt, std::vector<std::string> tokens);
        Client *FindClientInChannel(std::string target, Channel *channel);
		void applyAddForAllChannels(Client *client, std::vector<std::string> chnls, short mode, std::string param);
		void applyRmForAllChannels(Client *client, std::vector<std::string> chnls, short mode, std::string param);
        std::vector<std::string> CheckAndSeparate(Client *client, std::vector<std::string> tokens);
        void    CheckMembership(Client *client, Channel *channel);
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