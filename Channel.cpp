#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel()
{
}

Channel::Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers)
{
    _name = name;
    _topic = topic;
    _mode = mode;
    _maxUsers = maxUsers;
    _isPrivate = false;
}

Channel::Channel(const std::string &name)
{
    _name = name;
    _topic = "";
    _mode = "";
    _maxUsers = 0;
    _isPrivate = false;
}

bool Channel::isOnChannel(Client *client) const
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
        {
            return true; 
        }
    }
    return false; 
}

bool Channel::isEmpty() const
{
    if (_clients.size() == 0)
        return true;
    return false;
}

bool Channel::isPrivate() const
{
    return _isPrivate;
}

void Channel::removeClient(Client *client)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
        {
            _clients.erase(_clients.begin() + i);
            return;
        }
    }
}

std::string Channel::getUsersList() const
{
    std::string userList;
    for (size_t i = 0; i < _clients.size(); i++)
    {
        userList += _clients[i]->getNickname() + " ";
    }
    return userList;
}


void Channel::setOperator(Client *client)
{
    _operators.push_back(client->getNickname());
}

void Channel::addClient(Client *client)
{
    _clients.push_back(client);
}

Channel::~Channel()
{
}

std::string Channel::getName() const
{
    return _name;
}

std::string Channel::getTopic() const
{
    return _topic;
}

std::string Channel::getMode() const
{
    return _mode;
}

int Channel::getMaxUsers() const
{
    return _maxUsers;
}

std::string Channel::getChannelName() const
{
    return _name;
}
std::vector<Client *> Channel::getClients() const
{
    return _clients;
}

std::vector<std::string> Channel::getBannedUsers() const
{
    return _bannedUsers;
}

std::vector<std::string> Channel::getOperators() const
{
    return _operators;
}

Client *Channel::getOwner() const
{
    return _owner;
}
