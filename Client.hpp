#ifndef CLIENT_HPP
#define CLIENT_HPP
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
#define RED    "\033[31m"
#define GREEN  "\033[32m"
#define YELLOW "\033[33m"
#define RESET  "\033[0m"

class Server;
class Client
{
    private:
        std::string     _nickname;
        std::string     _username;
		std::string		_claimedPswd;
        bool            _isOperator;
		bool			_isConnected;
		bool			_isAuthenticated;
        friend class Server;
    public:
        Client();
        Client(const std::string &nickname, const std::string &username, bool isOperator);
        ~Client();
        std::string getNickname() const;
        std::string getUsername() const;
		std::string	getClaimedPsswd(void) const;
		void		setNickname(std::string nickname);
		void		setUsername(std::string username);
		void	setConnection(bool connection);
		void	setClaimedPsswd(std::string passwd);
		void	setAuthentication(bool authentication);
		bool	isAuthenticated(void) const;
		bool	isConnected(void) const;
        bool 	isOperator() const;

};

#endif