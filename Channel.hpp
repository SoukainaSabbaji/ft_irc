#ifndef CHANNEL_HPP
#define	CHANNEL_HPP
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
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

class Client;

class Channel
{
    private:
        std::string             _name;
        std::string             _topic;
        std::string             _mode;
        std::string             _key;
        int                     _maxUsers;
        bool                    _isPrivate;
        Server                  *_server;
        Client                  *_owner;
        std::vector<Client*>     _clients;
        std::vector<std::string> _bannedUsers;
        std::vector<std::string> _operators;
        std::vector<std::string> _invitedUsers;
        
    public:
        Channel();
        Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers);
        Channel(const std::string &name);
        ~Channel();
        std::string getName() const;
        std::string getTopic() const;
        std::string getMode() const;
        int getMaxUsers() const;
        Client *getOwner() const;
        std::vector<Client*> getClients() const;
        std::vector<std::string> getBannedUsers() const;
        std::vector<std::string> getOperators() const;
        std::string getChannelName() const;
        std::string getUsersList() const;
        std::string  getKey() const;
        bool isOnChannel(Client *client) const;
        bool isEmpty() const;
        bool isFull() const;
        void removeClient(Client *client);
        void setOperator(Client *client);
        bool isPrivate() const;
        bool isInvited(Client *client) const;
        bool isBanned(Client *client) const;
        void AddMember(Client *client, std::string password);
        void SendJoinReplies(Client *client);

};

#endif