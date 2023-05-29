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
#include "../includes/Server.hpp"

#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"
#define INV		0
#define TPC		1
#define KEY		2
#define	LIM		3

class Client;
class Server;

class Channel
{
    private:
        std::string             _name;
        std::string             _topic;
        std::string             _mode;
        std::string             _key;
        int                     _maxUsers;
        bool                    _isPrivate;
        Client                  *_owner;
        std::vector<std::string> _bannedUsers;
        std::vector<std::string> _operators;
        std::vector<std::string> _invitedUsers;
        short					 _chnMode;
    public:
        std::vector<Client*>     _clients;
        Server                  *_server;
        Channel();
        Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers);
        Channel(const std::string &name, Client *owner);
        ~Channel();
        std::string getName() const;
        std::string getTopic() const;
        short getMode() const;
        int getMemberCount() const; 
        int getMaxUsers() const;
        Client *getOwner() const;
        std::vector<Client*> getClients() const;
        std::vector<std::string> getBannedUsers() const;
        std::vector<std::string> getOperators() const;
        std::string getChannelName() const;
        std::string getUsersList() const;
        std::string  getKey() const;
        void setTopic(Client *client, const std::string &topic, int token_flag);
		void setLimit(size_t limit);
        bool isOnChannel(Client *client) const;
        bool isEmpty() const;
        bool isFull() const;
        void removeClient(Client *_client, std::string reason);
        void setOperator(Client *client);
		bool removeOperator(Client *client);
		bool setMode(int mode);
		bool removeMode(int mode);
        bool isPrivate() const;
        bool isInvited(Client *client) const;
        bool isBanned(Client *client) const;
        void AddMember(Client *client, std::string password);
        void SendJoinReplies(Client *client);
        void BroadcastJoinMessage(Client *client);
        void CheckJoinErrors(Client *client, std::string password);
        bool CheckOperator(Client *client);
        bool CheckMember(Client *client);
        void TheBootlegBroadcast(std::string message);
        void destroyMember(Client *client);
        void AddInvitedMember(Client *client, Client *invited);
        Channel &operator=(const Channel &channel);

};

#endif