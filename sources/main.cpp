#include "../includes/Server.hpp"

#include "../includes/Client.hpp"


bool validatePort(const std::string &port)
{
    for (std::string::const_iterator it = port.begin(); it != port.end(); ++it)
    {
        if (!isdigit(*it))
            return false;
    }
    int portValue = atoi(port.c_str());
    return (portValue >= 1 && portValue <= 65535);
}

bool validatePassword(const std::string &password)
{
	size_t	passLen;

	passLen = password.length();
    return (passLen >= 8 && passLen <= 16);
}

int main(int argc , char **argv)
{
    if (argc != 3)
    {
        std::cout << RED << "Usage: " << argv[0] << " <port> <password>" << RESET << std::endl;
        return 1;
    }
    std::string port = argv[1];
    std::string pass = argv[2];
    if (!validatePort(port))
    {
        std::cout << RED << "Invalid port" << RESET << std::endl;
        return 1;
    }
    else if (!validatePassword(pass))
    {
        std::cout << RED << "Invalid password" << RESET << std::endl;
        return 1;
    }
    else 
    {
        Server server(atoi(port.c_str()), pass);
    }
}