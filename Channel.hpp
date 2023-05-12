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
        int                     _maxUsers;
        bool                    _isPrivate;
        Client                  *_owner;
        std::vector<Client*>     _clients;
        std::vector<std::string> _bannedUsers;
        std::vector<std::string> _operators;
    public:
        Channel();
        Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers);
        ~Channel();
        std::string getName() const;
        std::string getTopic() const;
        std::string getMode() const;
        int getMaxUsers() const;
        Client *getOwner() const;
        std::vector<Client*> getClients() const;
        std::vector<std::string> getBannedUsers() const;
        std::vector<std::string> getOperators() const;

};

#endif