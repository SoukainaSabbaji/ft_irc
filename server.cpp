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
void Server::processCommand(Client *client, std::vector<std::string> tokens)
{
    // compare tokens[0] to all commands
    if (tokens.empty())
        return;
    const std::string &command = tokens[0];
    if (command == "NICK" || command == "nick")
        _nickCommand(client, tokens);
}

std::string Server::normalizeLineEnding(std::string &str)
{
    std::string nstring = str;
    nstring.erase(std::remove(nstring.begin(), nstring.end(), '\r'), nstring.end());
    return nstring;
}

void Server::parseCommand(Client *client, std::string &command)
{
    std::vector<std::string> tokens;
    std::string token;
    std::string nstring = normalizeLineEnding(command);
    std::istringstream tokenStream(nstring);
    if (nstring.find('\n') != std::string::npos && nstring.size() > 1)
    {
        while (std::getline(tokenStream, token, ' '))
        {
            tokens.push_back(token);
        }
    }
    std::cout << tokens[0] << std::endl;
    processCommand(client, tokens);
}

bool Server::readFromClient(Client *client)
{
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, sizeof(buffer));

    std::string message; // Accumulate the complete message from multiple buffer reads

    while (true)
    {
        int len = recv(client->getFd(), buffer, sizeof(buffer) - 1, 0);
        if (len > 0)
        {
            std::string partialMessage(buffer, len);
            message += partialMessage;
            if (partialMessage.find('\n') != std::string::npos)
            {
                // Complete message received
                std::cout << "Received message from client " << client->getFd() << ": " << message << std::endl;
                parseCommand(client, message);
                break;
            }
            else
            {
                // Partial message received, request more data
                send(client->getFd(), "Continue by pressing \\n\n", 24, 0);
            }
        }
        else if (len == 0)
        {
            std::cout << "Client: " << client->getFd() << " disconnected" << std::endl;
            close(client->getFd());
            removeClient(client->getFd());
            return false;
        }
        else
        {
            std::cout << "Error reading from client " << client->getFd() << std::endl;
            close(client->getFd());
            removeClient(client->getFd());
            return false;
        }
    }
    return true;
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
            std::cout << GREEN << "New client connected" << RESET << std::endl;
        }
        for (size_t i = 1; i < _fdsVector.size(); i++)
        {
            Client *client = _clients[_fdsVector[i].fd];
            client->setFd(_fdsVector[i].fd);
            if (_fdsVector[i].revents & POLLIN)
            {
                if (!readFromClient(client))
                    break;
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
