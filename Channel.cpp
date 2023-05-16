#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

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


void    Channel::SendJoinReplies(Client *client)
{
    this->_server->sendMessage(NULL, client, RPL_TOPIC, 0, " " + this->getChannelName() + " :" + this->getTopic());
    this->_server->sendMessage(NULL, client, RPL_NAMREPLY, 0, " = " + this->getChannelName() + " :" + this->getUsersList());
    this->_server->sendMessage(NULL, client, RPL_ENDOFNAMES, 0, " " + this->getChannelName() + " :End of NAMES list");
    this->_server->sendMessage(NULL, client, RPL_MOTDSTART, 0, " :- " + this->getChannelName() + " Message of the day - ");
    this->_server->sendMessage(NULL, client, RPL_MOTD, 0, " :- " + this->getChannelName() + "  " + this->getTopic());
    this->_server->sendMessage(NULL, client, RPL_ENDOFMOTD, 0, " :End of MOTD command");
}


void    Channel::AddMember(Client *client, std::string password)
{
    //check if channel is invite mode and if client is invited
    if (this->getMode() == "i" && !this->isInvited(client))
    {
        this->_server->sendMessage(NULL, client, ERR_INVITEONLYCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+i)");
        return;
    }
    if (this->isBanned(client))
    {
        this->_server->sendMessage(NULL, client, ERR_BANNEDFROMCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+b)");
        return;
    }
    if (this->isFull())
    {
        this->_server->sendMessage(NULL, client, ERR_CHANNELISFULL, 0, " " + this->getChannelName() + " :Cannot join channel (+l)");
        return;
    }
    if (password != "" && password != this->getKey() && this->getMode() == "k")
    {
        this->_server->sendMessage(NULL, client, ERR_BADCHANNELKEY, 0, " " + this->getChannelName() + " :Cannot join channel (+k)");
        return;
    }
    if (this->isOnChannel(client))
    {
        this->_server->sendMessage(NULL, client, ERR_USERONCHANNEL, 0, client->getNickname() + " "+ this->getName() + " :is already on channel");
        return;
    }
    else 
    {
        _clients.push_back(client);
        if (this->getMode() == "o" && this->isEmpty())
        {
            this->setOperator(client);
        }
        this->_server->BroadcastMessage(NULL, client, ":" + client->getNickname() + "!~" + client->getUsername() + "@localhost" +  " JOIN " + this->getChannelName());
        this->SendJoinReplies(client);
    }
}
std::string  Channel::getKey() const
{
    return _key;
}

bool Channel::isFull() const
{
    if (_clients.size() == _maxUsers)
        return true;
    return false;
}

std::string Channel::getUsersList() const
{
    std::string userList;
    for (size_t i = 0; i < _clients.size(); i++)
    {
        std::string name;
        for (size_t j = 0; j < _operators.size(); j++)
        {
            if (_clients[i]->getNickname() == _operators[j])
            {
                name = "@" + _clients[i]->getNickname();
                break;
            }
            else
            {
                name = _clients[i]->getNickname();
            }
        }
        userList += name + " ";
    }
    return userList;
}

bool Channel::isInvited(Client *client) const
{
    for (size_t i = 0; i < _invitedUsers.size(); i++)
    {
        if (_invitedUsers[i] == client->getNickname())
        {
            return true;
        }
    }
    return false;
}

bool Channel::isBanned(Client *client) const
{
    for (size_t i = 0; i < _bannedUsers.size(); i++)
    {
        if (_bannedUsers[i] == client->getNickname())
        {
            return true;
        }
    }
    return false;
}

void Channel::setOperator(Client *client)
{
    _operators.push_back(client->getNickname());
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
