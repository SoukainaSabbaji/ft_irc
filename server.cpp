#include "Server.hpp"
#include "Client.hpp"

//********************** - Exceptions - **********************//

const char *Server::InvalidSocketFd::what() const throw()
{
    return "Invalid socket file descriptor";
}

const char *Server::FcntlError::what() const throw()
{
    return "Error setting socket to non-blocking";
}

const char *Server::SetsockoptError::what() const throw()
{
    return "Error setting socket options";
}

const char *Server::BindError::what() const throw()
{
    return "Error binding socket";
}

const char *Server::ListenError::what() const throw()
{
    return "Error listening socket";
}

//********************** - Private methods - **********************//

void Server::removeClient(int client_fd)
{
    std::map<int, Client*>::iterator it = _clients.find(client_fd);
    if (it != _clients.end())
    {
        delete it->second;
        _clients.erase(it);
    }
}


void Server::sendMessage(int fd, std::string message)
{
    if (fd < 0)
        return;
    if (send(fd, message.c_str(), message.size(), 0) < 0)
        std::cerr << "Error sending message to client" << std::endl;
}

void Server::_nickCommand(Client *client, std::vector<std::string> tokens)
{
    if (tokens.size() < 2)
    {
        // send error message to client
        sendMessage(client->getFd(), "ERR_NICKNAME_MISSING");
        return;
    }
}

Channel *Server::_findChannel(std::string channelName) const
{
    std::vector<Channel*>::const_iterator i;
    for(i = _channels.cbegin(); i != _channels.cend(); i++)
    {
        if ((*i)->getChannelName() == channelName)
                return (*i);
    }
    return (NULL);
}

void    Server::findChannelAndSendMessage(Client *client, std::string channelName, std::string message)
{
    Channel *target = _findChannel(channelName); 
    if (target)
    {
        std::vector<Client*> clients = target->getClients();
        while (!clients.empty())
        {
            sendMessage(clients.back()->getFd(), ":" + client->getNickname() + " PRIVMSG " + channelName + " :" + message + "\r\n");
            clients.pop_back();
        }
    }
}

void    Server::findTargetsAndSendMessage(Client *client, std::vector<std::string> recipients, std::string message)
{
    while (!recipients.empty())
    {
        std::string recipient = recipients.back();
        //find if recipient is a channel
        if (recipient[0] == '#')
            findChannelAndSendMessage(client, recipient, message);
    }

}

//PRIVMSG command 
void Server::_privmsgCommand(Client *client, std::vector<std::string> tokens)
{
    //check number of parameters
    if (tokens.size() < 3)
    {
        sendMessage(client->getFd(), ":localhost 461 " + (client->getNickname().empty() ? "*" : client->getNickname()) + " " + tokens[1] +  " :Not enough parameters\r\n");
        return;
    }
    //fetch target and message
    std::string target = tokens[1];
    std::string message = tokens[2];
    std::vector<std::string> recipients;
    //separate recoipients
    if (target.find(',') != std::string::npos)
    {
        std::istringstream tokenStream(target);
        std::string token;
        while (std::getline(tokenStream, token, ','))
        {
            recipients.push_back(token);
        }
    }
    else
    {
        recipients.push_back(target);
    }
    //send message to all recipients
    //find targets and send message
    findTargetsAndSendMessage(client, recipients, message);
}

void Server::processCommand(Client *client, std::vector<std::string> tokens)
{
    //remove \n from last token if it exists
    if (!tokens.empty())
    {
        std::string& lastMessage = tokens.back();
        if (!lastMessage.empty() && lastMessage.back() == '\n')
        {
            lastMessage.pop_back();
        }
    }
    // std::cout << "--" << tokens[0] << "--" << std::endl;
    if (tokens.empty())
        return;
    const std::string &command = tokens[0];
    if (command == "NICK" || command == "nick")
        _nickCommand(client, tokens);
    else if (command == "PRIVMSG" || command == "privmsg")
        _privmsgCommand(client, tokens);
}

std::string Server::normalizeLineEnding(std::string &str)
{
    std::string nstring = str;
    nstring.erase(std::remove(nstring.begin(), nstring.end(), '\r'), nstring.end());
    return nstring;
}

//parsing a single command from a client
void Server::parseCommand(Client *client, std::string &command)
{
    std::vector<std::string> tokens;
    std::string token;
    std::string nstring = normalizeLineEnding(command);
    std::istringstream tokenStream(nstring);
    //line ending is normalized  from windows style to unix style
    // and i then look for \n meaning that i have a full command
    if (nstring.find('\n') != std::string::npos && nstring.size() > 1)
    {
        // i split the command into tokens
        while (std::getline(tokenStream, token, ' '))
        {
            tokens.push_back(token);
        }
    }
    //process the command
    processCommand(client, tokens);
}

void Server::handleClient(Client *client)
{
    std::cout << "Handling client " << client->getFd() << std::endl;
    std::string message = readFromClient(client->getFd());
    if (message.empty())
        return;
    parseCommand(client, message);
}

std::string Server::readFromClient(int client_fd)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    std::cout << "Reading from client " << client_fd << std::endl;
    int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	std::string message = buffer;
    if (len > 0)
    {
        std::cout << "Received message from client : " << client_fd << ": " << message << std::endl;
        
    }
    else if (len == 0)
    {
        std::cout << "Client :" << client_fd << " disconnected" << std::endl;
        close(client_fd);
        removeClient(client_fd);
        return "";
    }
    // else
    // {
    //     std::cout << "Error reading from client " << client_fd << std::endl;
    //     close(client_fd);
    //     removeClient(client_fd);
    //     return "";
    // }
    return message;
}

void Server::InitSocket()
{
    struct sockaddr_in server_addr;

    this->_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
        throw InvalidSocketFd();

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        throw FcntlError();
        close(_fd);
    }
    int opt = 1;
    if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw SetsockoptError();
        close(_fd);
    }
    if (bind(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        throw BindError();
        close(_fd);
    }
    if (listen(_fd, 5) < 0)
    {
        throw ListenError();
        close(_fd);
    }
}

//********************** - Constr destr and getters - **********************//
Server::Server() : _fd(-1), _port(-1), _running(false), _password("")
{
}

Server::Server(int port, const std::string &password) : _fd(-1), _port(port), _running(true), _password(password)
{
    this->InitSocket();
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct pollfd client_poll_fd;
    pollfd server_poll_fd;
    int client_fd;

    server_poll_fd.fd = _fd;
    server_poll_fd.events = POLLIN;
    _fdsVector.push_back(server_poll_fd);
    while (this->_running == true)
    {
        if (poll(_fdsVector.data(), _fdsVector.size(), -1) < 0)
        {
            std::cout << RED << "Error polling" << RESET << std::endl;
            break;
        }
        if (_fdsVector[0].revents & POLLIN)
        {
            client_fd = accept(_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0)
            {
                std::cout << RED << "Error accepting client" << RESET << std::endl;
                break;
            }
            client_poll_fd.fd = client_fd;
            client_poll_fd.events = POLLIN;
            _fdsVector.push_back(client_poll_fd);
            _clients.insert(std::pair<int, Client *>(client_fd, new Client()));
			_clients[client_fd]->setConnection(true);
            std::cout << GREEN << "New client connected" << RESET << std::endl;
        }
        for (size_t i = 1; i < _fdsVector.size(); i++)
        {
            client_fd = _fdsVector[i].fd; 
            if (_fdsVector[i].revents & POLLIN)
            {
			    // if (_clients[client_fd] && _clients[client_fd]->isConnected() && !_clients[client_fd]->isAuthenticated())
			    	// authenticateUser(client_fd);
                std::cout << "Client " << client_fd << " is ready to read" << std::endl;
                Client *client = _clients[_fdsVector[i].fd];
                client->setFd(_fdsVector[i].fd);
                handleClient(client);
            }
        }
    }
}

// remind me to add the search for nicknames already in use.
bool Server::authenticateUser(int client_fd)
{
	std::string					message;
	std::vector<std::string>	args;
	std::string					holder;

	message = readFromClient(client_fd);
	std::stringstream	tmp(message);
	while (std::getline(tmp, holder, ' '))
		args.push_back(holder);
	if (args.size() < 2)
	{
		send(client_fd, "ERR_NEEDMOREPARAMS\n\r", 21, MSG_EOF);
		return (false);
	}
	if (message.find("PASS ") == 0)
	{
		_clients[client_fd]->setClaimedPsswd(message.substr(5, message.length() - 1));
		if (this->_password != _clients[client_fd]->getClaimedPsswd())
			send(client_fd, "ERR_PASSWDMISMATCH\n\r", 21, -1);
		return (false);
	}
	else if (message.find("USER ") == 0 || message.find("NICK ") == 0)
	{
		if (message.find("USER ") == 0)
			_clients[client_fd]->setUsername(args[1]);
		else
			_clients[client_fd]->setNickname(args[1]);
	}
	if (!_clients[client_fd]->getNickname().empty() && !_clients[client_fd]->getUsername().empty())
	{
		if (_clients[client_fd]->getClaimedPsswd() == this->_password)
		{
			return (_clients[client_fd]->setAuthentication(true), true);
		}
	}
	return (false);
}

Server::~Server()
{
    close(_fd);
}

int Server::getFd() const
{
    return _fd;
}

int Server::getPort() const
{
    return _port;
}

std::string Server::getPassword() const
{
    return _password;
}

bool Server::isRunning() const
{
    return _running;
}

//example of the error handling
// :localhost 434 ssabbaji :Pass is not set