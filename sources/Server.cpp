#include "../includes/Server.hpp"
#include "../includes/Client.hpp"


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
    std::map<int, Client *>::iterator it = _clients.find(client_fd);
    Client *client = _clients[client_fd];
	for(size_t i = 0; i < _fdsVector.size(); ++i)
	{
		if (client->getFd() == _fdsVector[i].fd)
		{
			pollfd tmp;
			tmp = _fdsVector[i];
			_fdsVector[i] = _fdsVector[_fdsVector.size() - 1];
			_fdsVector[_fdsVector.size() - 1] = tmp;
			_fdsVector.pop_back();
			close(client->getFd());
			break ;
		}
	}
    if (!client->getNickname().empty() && _nicknames[client->getNickname()])
        _nicknames.erase(client->getNickname());
    if (it != _clients.end())
    {
        delete it->second;
        _clients.erase(it);
    }
}

void Server::checkAndAuth(Client *clt)
{

    if (!clt->isAuthenticated() && !clt->getNickname().empty() && !clt->getUsername().empty() && clt->getClaimedPsswd() == this->_password)
    {
        clt->setAuthentication(true);
        std::cout << YELLOW << clt->getNickname() << GREEN << " authenticated" << RESET << std::endl;
        sendMessage(NULL, clt, 0, 1, " WELCOME TO THE BEST IRC SERVER MADE WITH LOVE BY SOUKI && SIXIE WE NAMED SOUKIXIE AS IT'S A TEAM WORK AND WE ARE THE BEST TEAM EVER");
        sendMessage(NULL, clt, 0, 2, " your host is " + this->_serverName + ", running the ver 0.0.1");
        sendMessage(NULL, clt, 0, 3, " this server Was creates at " + this->creationDate);
        sendMessage(NULL, clt, 0, 4, " AHAHA this feels like HELLO WORLD");
    }
}

/// @brief returns the ip of the client if NULL is passed returns the server domain
/// @param clt the client
char *Server::getAddr(Client *clt)
{
    struct sockaddr_in _host;
    socklen_t len;

    len = sizeof(struct sockaddr);
    getsockname(clt->getFd(), (sockaddr *)&_host, &len);
    return (inet_ntoa(_host.sin_addr));
}

///@brief this is the Bootleg send message because the first one only works in specific cases lol
void Server::theBootLegSendMessage(Client *dst, std::string msg)
{
    send(dst->getFd(), msg.c_str(), msg.length(), 0);
}

/// @brief this function sends a message from a source to a destination
/// it automatically adds the message prefixes to the message depending
/// on the source and destination, refer to rfc1459 section 2.3.1
/// @param src the source of the message client object pointer if it's from the client
/// NULL if it's the server
/// @param dst the destination of the message it is the client object NULL if it's a channel
/// @param ERRCODE the ERRCODE corespondig to the error faced
/// @param message the pure message that wants to be sent from
/// src to dst the prefixes will be added in the function.
void Server::sendMessage(Client *src, Client *dst, int ERRCODE, int RPLCODE, std::string message)
{
    // we need to find a way for RPL
    // RPL used to work idk what happened
    std::string _host;

    if (!src)
        _host = "irc.soukixie.local";
    else
        _host = getAddr(src);
    if (!src && RPLCODE)
        message = ":" + _host + " " + this->rplCodeToStr[RPLCODE] + " " + dst->getNickname() + " :" + message + "\r\n";
    else if (!src && ERRCODE == 0)
        message = ":" + this->_serverName + "!" + this->_serverName + "@" + _host + " PRIVMSG " + dst->getNickname() + " :" + message + "\r\n";
    else if (ERRCODE < 0)
        message = ":" + src->getNickname() + "!" + src->getUsername() + "@" + _host + " NOTICE " + dst->getNickname() + " " + message + "\r\n";
    else if (!ERRCODE && !RPLCODE)
        message = ":" + src->getNickname() + "!" + src->getUsername() + "@" + _host + " PRIVMSG " + dst->getNickname() + " " + message + "\r\n"; // works perfect for private messages can not send messages from server
    else
        message = ":" + _host + " " + this->errCodeToStr[ERRCODE] + " " + (dst->getNickname().empty() ? "*" : dst->getNickname()) + message + "\r\n"; // still not working
    send(dst->getFd(), message.c_str(), message.length(), 0);
}

void toLower(std::string &str)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        str[i] = std::tolower(str[i]);
    }
}

void Server::processCommand(Client *client, std::vector<std::string> tokens)
{
    if (tokens.empty())
        return;
    for (size_t i = 0; i < tokens.size(); i++)
        std::cout << "--" << tokens[i] << "--" << std::endl;

    std::string &command = tokens[0];
    toLower(command);
    typedef void (Server::*CommandFunction)(Client *, std::vector<std::string>);
    std::map<std::string, CommandFunction> commandMap;
    commandMap["nick"] = &Server::_nickCommand;
    commandMap["user"] = &Server::_userCommand;
    commandMap["pass"] = &Server::_passCommand;
    commandMap["privmsg"] = &Server::_privMsgCommand;
    commandMap["notice"] = &Server::_privMsgCommand; 
    commandMap["join"] = &Server::_joinCommand;
    commandMap["list"] = &Server::_listCommand;
    commandMap["kick"] = &Server::_kickCommand;
    commandMap["part"] = &Server::_partCommand;
    commandMap["topic"] = &Server::_topicCommand;
    commandMap["invite"] = &Server::_inviteCommand;
    commandMap["bot"] = &Server::_botCommand;
    commandMap["quit"] = &Server::_quitCommand;
    commandMap["ping"] = &Server::_pingCommand;
	commandMap["mode"] = &Server::_modeCommand;

    std::map<std::string, CommandFunction>::iterator it = commandMap.find(command);
    if (it != commandMap.end())
    {
        CommandFunction func = it->second;
        (this->*func)(client, tokens);
    }
    else
    {
        if (command == "pong")
            return;
        sendMessage(NULL, client, ERR_UNKNOWNCOMMAND, 0, " " + command + " :Unknown command");
    }
}

std::string Server::readFromClient(Client *client)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    int len = recv(client->getFd(), buffer, 500, 0);
    if (len <= 0)
    {
        if (len == 0)
        {
            std::cout << RED << "Client: " << client->getFd() << " disconnected" << RESET << std::endl;
            close(client->getFd());
            removeClient(client->getFd());
        }
        return "";
    }
    std::string command = buffer;
    client->_buffer.append(command);
    client->_buffer = normalizeLineEnding(client->_buffer);
    if (client->_buffer.find('\n') != std::string::npos && client->_buffer.size() > 1)
    {
        parseCommand(client);
        client->_buffer.clear();
    }
    return command;
}

void Server::initCode()
{
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOSUCHNICK, "401"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOSUCHCHANNEL, "403"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CANNOTSENDTOCHAN, "404"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_TOOMANYCHANNELS, "405"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_WASNOSUCHNICK, "406"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_TOOMANYTARGETS, "407"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOORIGIN, "409"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NORECIPIENT, "411"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTEXTTOSEND, "412"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTOPLEVEL, "413"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_WILDTOPLEVEL, "414"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_UNKNOWNCOMMAND, "421"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_FILEERROR, "424"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NONICKNAMEGIVEN, "431"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_ERRONEUSNICKNAME, "432"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NICKNAMEINUSE, "433"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERNOTINCHANNEL, "441"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOTONCHANNEL, "442"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERONCHANNE, "443"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOLOGIN, "444"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NEEDMOREPARAMS, "461"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_ALREADYREGISTRED, "462"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOPERMFORHOST, "463"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_PASSWDMISMATCH, "464"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_YOUREBANNEDCREEP, "465"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CHANNELISFULL, "471"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_BANNEDFROMCHAN, "474"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_BADCHANNELKEY, "475"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_NOPRIVILEGES, "481"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_CHANOPRIVSNEEDED, "482"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERSDONTMATCH, "502"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_USERONCHANNEL, "443"));
    this->errCodeToStr.insert(std::pair<int, std::string>(ERR_INVITEONLYCHAN, "473"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_WELCOME, "001"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_YOURHOST, "002"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_CREATED, "003"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_MYINFO, "004"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_TOPIC, "332"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_NAMREPLY, "353"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_ENDOFNAMES, "366"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_MOTDSTART, "375"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_MOTD, "372"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_ENDOFMOTD, "376"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_NOTOPIC, "331"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_TOPICWHOTIME, "333"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_INVITED, "353"));
    this->rplCodeToStr.insert(std::pair<int, std::string>(RPL_INVITING, "341"));
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

bool Server::nickAvailable(std::string nick)
{
    return (this->_nicknames.count(nick) == 0);
}

//********************** - Constr destr and getters - **********************//
Server::Server() : _fd(-1), _port(-1), _running(false), _password("")
{
}

std::string Server::getDate(void)
{
    time_t rawtime;
    char *tmp;
    std::string now;

    time(&rawtime);
    tmp = ctime(&rawtime);
    now = tmp;
    now = now.substr(0, now.size() - 1);
    return (now);
}

Server::Server(int port, const std::string &password) : _fd(-1), _port(port), _running(true), _password(password)
{
    this->_serverName = "soukixie"; // 9 characters and a merge between our names ^_^
    this->creationDate = this->getDate();
    this->_nicknames.insert(std::pair<std::string, Client *>(this->_serverName, new Client));
    this->InitSocket();
    this->initCode();
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct pollfd client_poll_fd;
    pollfd server_poll_fd;
    int client_fd;
    std::string rawMessage;

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
            int client_fd = _fdsVector[i].fd;
            if (_fdsVector[i].revents & POLLIN)
            {
                Client *client = _clients[client_fd];
                client->setFd(client_fd);
                readFromClient(client);

                if (this->_clients.count(client_fd) > 0)
                {
                    size_t pos = client->_buffer.find("\n");
                    if (pos != std::string::npos)
                        client->_buffer.erase(0, pos + 1);
                }
            }
        }
    }
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

// example of the error handling
//  :localhost 434 ssabbaji :Pass is not set

// rpl message format
//: server_name reply_code target :reply_message

// error message format
//: server_name ERROR error_code target :error_message

//_privMsgCommand format
//: sender_nick!sender_user@sender_host PRIVMSG target :message_text

// send an error when no text is sent through notice , no user not found error