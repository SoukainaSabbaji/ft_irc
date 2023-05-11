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

void Server::readFromClient(int client_fd)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    int len = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (len > 0)
    {
        std::string message(buffer, len);
        std::cout << "Received message from client " << client_fd << ": " << message << std::endl;
        //further processing of the message me thinks 
    }
    else if (len == 0)
    {
        std::cout << "Client " << client_fd << " disconnected" << std::endl;
        close(client_fd);
        //remove the client 
    }
    else
    {
        std::cout << "Error reading from client " << client_fd << std::endl;
        close(client_fd);
        //remove the client 
    }
}



void    Server::InitSocket()
{
    struct sockaddr_in server_addr;

    this->_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
        throw InvalidSocketFd();

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    //set the server to be non-blocking using fcntl
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
    { 
        throw FcntlError();
        close(_fd);
    }
    //    set the server to be reusuable using setsockopt
    int opt = 1;
    if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt)) < 0)
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
            std::cout << RED << "Error polling" << RESET <<  std::endl;
            break;
        }
        if (_fdsVector[0].revents & POLLIN)
        {
            client_fd = accept(_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd < 0)
            {
                std::cout << "Error accepting client" << std::endl;
                break;
            }
            client_poll_fd.fd = client_fd;
            client_poll_fd.events = POLLIN;
            _fdsVector.push_back(client_poll_fd);
            _clients.insert(std::pair<int, Client*>(client_fd, new Client()));
            std::cout << "New client connected" << std::endl;
        }
        for (size_t i = 1; i < _fdsVector.size(); i++)
        {
            if (_fdsVector[i].revents & POLLIN)
            {
                std::cout << "Client " << _fdsVector[i].fd << " sent a message" << std::endl;
                readFromClient(client_fd);
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
