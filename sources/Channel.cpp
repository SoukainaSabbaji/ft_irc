#include "../includes/Channel.hpp"
#include "../includes/Client.hpp"
#include "../includes/Server.hpp"

Channel::Channel(const std::string &name, const std::string &topic, const std::string &mode, int maxUsers)
{
    _name = name;
    _topic = topic;
    _mode = mode;
    _maxUsers = maxUsers;
    _isPrivate = false;
	_chnMode = 0;
}

Channel::Channel(const std::string &name, Client *owner)
{
    _name = name;
    _topic = "";
    _mode = "";
    _maxUsers = 100;
    _isPrivate = false;
    _owner = owner;
	_chnMode = 0;
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

void Channel::destroyMember(Client* _client)
{
    std::vector<Client*>::iterator clientIt = std::find(_clients.begin(), _clients.end(), _client);
    if (clientIt != _clients.end())
        _clients.erase(clientIt);
    std::vector<std::string>::iterator operatorIt = std::find(_operators.begin(), _operators.end(), _client->getNickname());
    if (operatorIt != _operators.end())
        _operators.erase(operatorIt);
    std::vector<std::string>::iterator invitedUserIt = std::find(_invitedUsers.begin(), _invitedUsers.end(), _client->getNickname());
    if (invitedUserIt != _invitedUsers.end())
        _invitedUsers.erase(invitedUserIt);
}


void Channel::removeClient(Client *_client, std::string reason)
{
    if (!CheckMember(_client))
    {
        this->_server->sendMessage(NULL, _client, ERR_NOTONCHANNEL, 0, " " + _name + " :You're not on that channel");
        return;
    }
    std::stringstream ss;
    ss << ":" << _client->getNickname() << "!~" << _client->getUsername() << "@localhost" << " PART " << _name << " " << reason << "\r\n";
    std::string message = ss.str();
    TheBootlegBroadcast(message);
    destroyMember(_client);
    if (this->_operators.size() == 0)
    {
        if (this->_clients.size())
            this->_operators.push_back((*_clients.begin())->getNickname());
    }
}


bool Channel::CheckOperator(Client *client)
{
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client->getNickname())
            return true;
    }
    return false;
}

bool    Channel::CheckMember(Client *client)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i] == client)
            return true;
    }
    return false;
}


void    Channel::SendJoinReplies(Client *client)
{
    std::string NamesReply = ":irc.soukixie.local 353 " + client->getNickname() + " = " + this->getChannelName() + " :" + this->getUsersList() + "\r\n";
    send(client->getFd(), NamesReply.c_str(), NamesReply.length(), 0);
    std::string EndOfNamesReply = ":irc.soukixie.local 366 " + client->getNickname() + " " + this->getChannelName() + " :End of /NAMES list" + "\r\n";
    send(client->getFd(), EndOfNamesReply.c_str(), EndOfNamesReply.length(), 0);
    if (this->getTopic() != "")
        this->_server->sendMessage(NULL, client, 0, RPL_TOPIC, " " + this->getChannelName() + " TOPIC :" + this->getTopic());
}

void    Channel::TheBootlegBroadcast(std::string message)
{
    for (size_t i = 0; i < this->getClients().size(); i++)
    {

        send(this->getClients()[i]->getFd(), message.c_str(), message.length(), 0);
    }
}


int Channel::getMemberCount() const
{
    return (_clients.size());
}

bool Channel::CheckJoinErrors(Client *client, std::string password)
{
    if ((this->getMode() >> INV) % 2 && !this->isInvited(client))
    {
        this->_server->sendMessage(NULL, client, ERR_INVITEONLYCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+i)");
        return false;
    }
    if (this->isBanned(client))
    {
        this->_server->sendMessage(NULL, client, ERR_BANNEDFROMCHAN, 0, " " + this->getChannelName() + " :Cannot join channel (+b)");
        return false;
    }
    if (this->isFull())
    {
        this->_server->sendMessage(NULL, client, ERR_CHANNELISFULL, 0, " " + this->getChannelName() + " :Cannot join channel (+l)");
        return false;
    }
    if ((this->getMode() >> KEY) % 2 && password != this->getKey() && password != "")
    {
        this->_server->sendMessage(NULL, client, ERR_BADCHANNELKEY, 0, " " + this->getChannelName() + " :Cannot join channel (+k)");
        return false;
    }
    return true;
}

void    Channel::AddMember(Client *client, std::string password)
{
    //check if channel is invite mode and if client is invited
    if (!CheckJoinErrors(client, password))
        return;
    if (this->isOnChannel(client))
    {
        this->_server->sendMessage(NULL, client, ERR_USERONCHANNEL, 0, " "+ this->getName() + " :is already on channel");
        return;
    }
    else 
    {
        //check if the client joined the maximum number of channels of not yet
        if (client->_nbrchannels == 10)
        {
            this->_server->sendMessage(NULL, client, ERR_TOOMANYCHANNELS, 0, " " + this->getChannelName() + " :You have joined too many channels");
            return;
        }
        client->_nbrchannels++;
        if (this->isEmpty())
        {
            this->setOperator(client);
            this->_owner = client;
        }
        _clients.push_back(client);
        BroadcastJoinMessage(client);
        this->SendJoinReplies(client);
    }
}

void    Channel::BroadcastJoinMessage(Client *client)
{
    std::string BroadcastMessage = ":" + client->getNickname() + "!~" + client->getUsername() + "@localhost" +  " JOIN :" + this->getChannelName() + "\r\n";
    std::vector<Client *> clients = this->getClients();
    for (size_t i = 0; i < clients.size(); i++)
    {
        Client *dst = clients[i];
        send(dst->getFd(), BroadcastMessage.c_str(), BroadcastMessage.length(), 0);
    }
}

std::string  Channel::getKey() const
{
    return _key;
}

bool Channel::isFull() const
{
    int size = _clients.size();
    if (size == _maxUsers)
        return true;
    return false;
}

std::string Channel::getUsersList() const
{
    std::string userList;
    for (size_t i = 0; i < _clients.size(); i++)
    {
        std::string name;
        if (_clients[i] == this->_owner)
            name = "@" + _clients[i]->getNickname();
        else
            name = _clients[i]->getNickname();
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

void    Channel::AddInvitedMember(Client *client, Client *invited)
{
    if (this->isOnChannel(invited))
    {
        this->_server->sendMessage(NULL, client, ERR_USERONCHANNEL, 0, " "+ this->getName() + " :is already on channel");
        return;
    }
    else 
        _invitedUsers.push_back(invited->getNickname());
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

bool Channel::removeOperator(Client *client)
{
	for (size_t i = 0; i <= _operators.size(); ++i)
	{
		if (i == _operators.size())
			return (false);
		if (_operators[i] == client->getNickname())
		{
			_operators[i] = _operators[_operators.size()];
			_operators[_operators.size()] = client->getNickname();
			_operators.pop_back();
			break ;
		} 
	}
	return (true);
}

bool	Channel::setMode(int mode)
{
	if (!(_chnMode >> mode) % 2)
		_chnMode += 1 << mode;
	return (true);
}

bool	Channel::removeMode(int mode)
{
	if ((_chnMode >> mode) % 2)
		_chnMode -= 1 << mode;
	return (true);
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

short Channel::getMode() const
{
    return _chnMode;
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

bool Channel::setLimit(size_t limit)
{
	if (!_maxUsers)
		_maxUsers = limit;
	else if (limit)
		return (false);
	return (true);
	
}

void Channel::setTopic(Client *client, const std::string &topic, int token_flag)
{
    time_t now = time(0);
    std::ostringstream oss;
    oss << now;
    std::string topic_time ;
    
    if (topic == " ")
    {
        this->_server->sendMessage(NULL, client, RPL_NOTOPIC, 0, " " + this->getChannelName() + " :No topic is set");
        return;
    }
    if ((getMode() >> TPC) % 2 && !CheckOperator(client))
    {
        this->_server->sendMessage(NULL, client, ERR_CHANOPRIVSNEEDED, 0, " " + this->getChannelName() + " :You're not channel operator");
        return;
    }
    if (token_flag)
    {
        _topic = topic;
        topic_time = oss.str();
        std::string topic_broadcast = ":" + client->getNickname() + "!~" + client->getUsername() + "@localhost" + " TOPIC " + this->getChannelName() + " " + topic + "\r\n";;
        TheBootlegBroadcast(topic_broadcast);
    }
    else 
    {
        if (_topic == "")
            this->_server->sendMessage(NULL, client, 0, RPL_NOTOPIC, " " + this->getChannelName() + " :No topic is set.");
        else
        {
            this->_server->sendMessage(NULL, client, 0, RPL_TOPIC, " " + this->getChannelName() + _topic);
            this->_server->sendMessage(NULL, client, 0, RPL_TOPICWHOTIME, " " + this->getChannelName() + " " + client->getNickname() + " " + topic_time);
        }
    }
}

//assignment operator overload
Channel &Channel::operator=(const Channel &rhs)
{
    if (this != &rhs)
    {
        this->_name = rhs._name;
        this->_topic = rhs._topic;
        this->_mode = rhs._mode;
        this->_maxUsers = rhs._maxUsers;
        this->_key = rhs._key;
        this->_owner = rhs._owner;
        this->_clients = rhs._clients;
        this->_bannedUsers = rhs._bannedUsers;
        this->_invitedUsers = rhs._invitedUsers;
        this->_operators = rhs._operators;
    }
    return *this;
}

void	Channel::setKey(std::string key)
{
	this->_key = key;
}
