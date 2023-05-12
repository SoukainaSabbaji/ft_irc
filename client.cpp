#include "Client.hpp"

Client::Client()
{
    _nickname = "";
    _username = "";
    _isOperator = false;
	_isConnected = false;
	_isAuthenticated = false;
}

Client::Client(const std::string &nickname, const std::string &username, bool isOperator)
{
    _nickname = nickname;
    _username = username;
    _isOperator = isOperator;
}

Client::~Client()
{
}

std::string Client::getNickname() const
{
    return _nickname;
}

std::string Client::getUsername() const
{
    return _username;
}

int Client::getFd() const
{
    return _fd;
}

void Client::setFd(int fd)
{
    _fd = fd;
}
void	Client::setNickname(std::string nickname)
{
	this->_nickname = nickname;
}

void	Client::setUsername(std::string username)
{
	this->_username = username;
}

bool Client::isOperator() const
{
    return _isOperator;
}

void	Client::setClaimedPsswd(std::string passwd)
{
	this->_claimedPswd = passwd;
}

void	Client::setConnection(bool connection)
{
	this->_isConnected = connection;
}

void	Client::setAuthentication(bool authentication)
{
	this->_isAuthenticated = authentication;
}

bool	Client::isAuthenticated(void) const
{
	return (this->_isAuthenticated);
}

bool	Client::isConnected(void) const
{
	return (this->_isConnected);
}

std::string	Client::getClaimedPsswd(void) const
{
	return (this->_claimedPswd);
}
//
