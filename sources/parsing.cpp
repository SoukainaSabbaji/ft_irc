#include "../includes/Server.hpp"

#include "../includes/Client.hpp"


std::string Server::normalizeLineEnding(std::string &str)
{
    std::string nstring = str;
    std::size_t pos = 0;
    while ((pos = nstring.find("\r\n", pos)) != std::string::npos)
        nstring.replace(pos, 2, "\n");
    return (nstring);
}

// parsing a single command from a client
void Server::parseCommand(Client *client)
{
    std::vector<std::string> tokens;
    std::string command = client->_buffer;
    std::string tmp = "";

    size_t i = 0;
    size_t len = command.length();
    while (i < len)
    {
        if (command[i] == ' ')
        {
            if (!tmp.empty() && tmp[0] != '\n')
                tokens.push_back(std::move(tmp));
            tmp.clear();
            while (i < len && command[i + 1] == ' ')
                i++;
        }
        else
            tmp += command[i];
        i++;
    }
    if (!tmp.empty() && tmp[0] != '\n')
        tokens.push_back(std::move(tmp));
    std::string &lastToken = tokens.back();
    if (!lastToken.empty() && lastToken.back() == '\n')
        lastToken.pop_back();
    processCommand(client, tokens);
}