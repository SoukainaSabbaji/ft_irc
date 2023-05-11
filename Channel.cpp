#include "Channel.hpp"

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

std::vector<Client*> Channel::getClients() const
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
