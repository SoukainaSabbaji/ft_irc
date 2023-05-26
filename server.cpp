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
    std::map<int, Client *>::iterator it = _clients.find(client_fd);
    Client *client = _clients[client_fd];
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
        std::cout << clt->getNickname() << " authenticated" << std::endl;
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

///@brief this is the Boot leg send message because the first one only works in specific cases lol

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

void Server::_userCommand(Client *client, std::vector<std::string> tokens)
{
    if (tokens.size() < 4)
    {
        // send error message to client
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " USER :Not enough parameters");
        return;
    }
    client->setUsername(tokens[1].substr(0, tokens[1].find('\r')));
    // sendMessage(NULL, client, 0, 0,"Username Set to " + tokens[1]);
    checkAndAuth(client);
}

// i hate this nested function
// we need to add more checks on the nickname as it has a format and length
void Server::_nickCommand(Client *client, std::vector<std::string> tokens)
{
    if (tokens.size() < 2)
    {
        // send error message to client
        sendMessage(NULL, client, ERR_NONICKNAMEGIVEN, 0, " :No nickname given");
        return;
    }
    std::string token = tokens[1].substr(0, tokens[1].find('\r'));
    if (!nickAvailable(token))
    {
        sendMessage(NULL, client, ERR_NICKNAMEINUSE, 0, " :" + token + " is already in use");
        return;
    }
    if (!client->getNickname().empty() && !client->isAuthenticated())
    {
        client->setNickname(token);
        this->_nicknames.insert(std::pair<std::string, Client *>(token, client));
        checkAndAuth(client);
    }
    else
    {
        _nicknames.erase(client->getNickname());
        client->setNickname(token);
        _nicknames.insert(std::pair<std::string, Client *>(token, client));
    }
}

void Server::_passCommand(Client *clt, std::vector<std::string> tokens)
{
    if (tokens.size() < 2)
    {
        // send error message to client
        sendMessage(NULL, clt, ERR_NEEDMOREPARAMS, 0, " NICK :Not enough parameters");
        return;
    }
    if (tokens[1].substr(0, tokens[1].find('\r')) != this->getPassword())
    {
        sendMessage(NULL, clt, ERR_PASSWDMISMATCH, 0, " Password incorrect");
        return;
    }
    clt->setClaimedPsswd(tokens[1].substr(0, tokens[1].find('\r')));
    // sendMessage(NULL, clt, 0, 0, "PASSWD SET ");
    checkAndAuth(clt);
}

Channel *Server::_findChannel(std::string channelName) const
{
    std::vector<Channel *>::const_iterator i;
    for (i = _channels.cbegin(); i != _channels.cend(); i++)
    {
        if ((*i)->getChannelName() == channelName)
            return (*i);
    }
    return (NULL);
}

void Server::BroadcastMessage(Client *client, Channel *target, const std::string &message)
{
    std::vector<Client *> clients = target->getClients();
    for (size_t i = 0; i < clients.size(); i++)
    {
        Client *clientTarg = clients[i];
        if (clientTarg != client)
            sendMessage(client, clientTarg, 0, 0, message);
    }
}

void Server::SendToRecipients(Client *client, std::vector<std::string> recipients, std::string message, std::string command)
{
    while (!recipients.empty())
    {
        if (nickAvailable(recipients.back()) && command == "privmsg")
        {
            sendMessage(NULL, client, ERR_NOSUCHNICK, 0, " " + recipients.back() + " :No such nick/channel");
            return;
        }
        else
        {
            Client *target = _nicknames[recipients.back()];
            if (command == "privmsg")
                sendMessage(client, target, 0, 0, message);
            else if (command == "notice")
                sendMessage(client, target, -1, 0, message);
            recipients.pop_back();
        }
    }
}

void Server::SendToRecipient(Client *client, std::vector<std::string> recipients, std::string message, bool isChannel, std::string command)
{
    if (isChannel)
    {
        Channel *target = _findChannel(recipients[0]);
        if (target)
            BroadcastMessage(client, target, message);
        else
        {
            // only display error message if the command is PRIVMSG
            if (command == "privmsg")
            {
                sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + recipients[0] + " :No such channel");
                return;
            }
        }
    }
    else
        SendToRecipients(client, recipients, message, command);
}

void Server::findTargetsAndSendMessage(Client *client, std::vector<std::string> recipients, std::string message, std::string command)
{
    // this function checks if the recipient is a channel or a user
    // and sends the message to the appropriate destination
    while (!recipients.empty())
    {
        std::string recipient = recipients.back();
        // find if recipient is a channel
        if (recipient[0] == '#')
            SendToRecipient(client, recipients, message, true, command);
        else
            SendToRecipient(client, recipients, message, false, command);
        recipients.pop_back();
    }
}

bool Server::nickAvailable(std::string nickname) const
{
    if (_nicknames.find(nickname) == _nicknames.end())
        return (true);
    return (false);
}

void Server::CheckAuthentication(Client *client)
{
    if (client->isAuthenticated())
        return;
    else
    {
        sendMessage(NULL, client, ERR_NOLOGIN, 0, client->getNickname() + " :User not logged in");
        return;
    }
}

/// @brief /PRIVMSG command
/// @param client  client that sent the command
/// @param tokens  command and parameters
void Server::_privMsgCommand(Client *client, std::vector<std::string> tokens)
{
    // check number of parameters
    CheckAuthentication(client);
    if (tokens.size() < 2)
    {
        sendMessage(NULL, client, ERR_NORECIPIENT, 0, " No recipient given " + tokens[0]);
        return;
    }
    else if (tokens.size() < 3)
    {
        sendMessage(NULL, client, ERR_NOTEXTTOSEND, 0, " No text to send");
        return;
    }
    // fetch target and message
    std::vector<std::string> recipients = SplitTargets(tokens[1]);
    std::string message = "";
    for (size_t i = 2; i < tokens.size(); ++i)
        message += tokens[i] + " ";
    findTargetsAndSendMessage(client, recipients, message, tokens[0]);
}

std::vector<std::string> Server::SplitTargets(std::string token)
{
    std::vector<std::string> recipients;
    std::string target = token;
    // separate recoipients
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
        recipients.push_back(target);
    return (recipients);
}

void Server::_joinCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 2)
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "JOIN :Not enough parameters");
        return;
    }
    std::string password = tokens[2];
    std::vector<std::string> channels = SplitTargets(tokens[1]);
    while (channels.size())
    {
        std::string channelName = channels.back();
        if (channelName[0] != '#' && channelName[0] != '&')
        {
            sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + channelName + " :No such channel");
            channels.pop_back();
            continue;
        }
        Channel *channel = _findChannel(channelName);
        if (!channel)
        {
            channel = new Channel(channelName, client);
            _channels.push_back(channel);
            channel->_server = this;
        }
        channel->AddMember(client, password);
        channels.pop_back();
    }
}

std::string ft_itoa(int num)
{
    std::string result;
    bool isNegative = false;

    if (num == 0)
    {
        result = "0";
        return result;
    }
    if (num < 0)
    {
        isNegative = true;
        num = -num;
    }
    while (num > 0)
    {
        int digit = num % 10;
        result = static_cast<char>('0' + digit) + result;
        num /= 10;
    }
    if (isNegative)
        result = "-" + result;
    return result;
}

void Server::_listCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    std::vector<std::string> channels;
    std::cout << tokens.size() << std::endl;
    if (tokens.size() > 1)
        channels = SplitTargets(tokens[1]);
    // Start of /LIST command message
    std::string StartMessage = ":irc.soukixie.local 321 " + client->getNickname() + "  :Start of /LIST command.\r\n";
    send(client->getFd(), StartMessage.c_str(), StartMessage.size(), 0);
    while (channels.size())
    {
        std::cout << "sending list 2" << std::endl;
        std::string channelName = channels.back();
        Channel *channel = _findChannel(channelName);
        if (!channel)
        {
            sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + channelName + " :No such channel");
            channels.pop_back();
            continue;
        }
        std::string message;
        std::stringstream ss;
        ss << ":irc.soukixie.local 322 " << client->getNickname() << " " << channel->getName() << ft_itoa(channel->getClients().size()) << " :";
        ss << channel->getTopic() << "\r\n";
        message = ss.str();
        send(client->getFd(), message.c_str(), message.size(), 0);
        // sendMessage(NULL, client, RPL_LIST, 0, " " + channel->getName() + " " + std::to_string(channel->getMemberCount()) + " :" + channel->getTopic());
        channels.pop_back();
    }
    sendMessage(NULL, client, RPL_LISTEND, 0, " :End of /LIST");
    // }
}

std::string getReason(std::vector<std::string> tokens)
{
    std::string reason;
    if (tokens.size() < 4)
        return ("");
    if (tokens[3][0] == ':')
    {
        for (size_t i = 3; i < tokens.size(); i++)
        {
            reason += tokens[i];
            if (i != tokens.size() - 1)
                reason += " ";
        }
    }
    else
        reason = tokens[3];
    return (reason);
}

void Server::DeleteEmptyChan(Channel *channel, std::vector<Channel *> _channels)
{
    // check if the channel is empty , if so delete it
    std::cout << "channel->getMemberCount(): " << channel->getMemberCount() << std::endl;
    if (channel->getClients().size() == 0)
    {
        std::vector<Channel *>::iterator it = std::find(_channels.begin(), _channels.end(), channel);
        _channels.erase(it);
        delete channel;
    }
}

void Server::_partCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 2)
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "PART :Not enough parameters");
        return;
    }
    // print list of channels in the server
    std::vector<std::string> channels = SplitTargets(tokens[1]);
    std::string reason = getReason(tokens);
    while (channels.size())
    {
        std::string channelName = channels.back();
        Channel *channel = _findChannel(channelName);
        if (!channel)
        {
            sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + channelName + " :No such channel");
            // sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, channelName + "No such channel");
            channels.pop_back();
            continue;
        }
        else
        {
            channel->removeClient(client, reason);
            DeleteEmptyChan(channel, _channels);
        }
        channels.pop_back();
    }
}

Client *Server::FindClientInChannel(std::string target, Channel *channel)
{
    std::vector<Client *> clients = channel->getClients();
    std::vector<Client *>::const_iterator it;

    for (it = clients.begin(); it != clients.end(); ++it)
    {
        if ((*it)->getNickname() == target)
            return *it;
    }
    return NULL;
}

void Server::YeetMember(Client *oper, Client *target, Channel *channel, std::string reason)
{
    if (!channel->CheckMember(target))
    {
        sendMessage(NULL, oper, ERR_USERNOTINCHANNEL, 0, target->getNickname() + " " + channel->getName() + " :They aren't on that channel");
        return;
    }
    std::stringstream ss;
    ss << ":" << oper->getNickname() << "!~" << oper->getUsername() << "@localhost"
       << " KICK " << channel->getName() << " " << target->getNickname() << " " << reason << "\r\n";
    std::string message = ss.str();
    channel->TheBootlegBroadcast(message);
    channel->destroyMember(target);
}

void Server::_kickCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 3)
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "KICK :Not enough parameters");
        return;
    }
    // split channels and targets to kick
    std::vector<std::string> channels = SplitTargets(tokens[1]);
    std::vector<std::string> targets = SplitTargets(tokens[2]);
    std::string reason = getReason(tokens);
    // check if the client requesting the kick is an operator in the channel
    while (channels.size())
    {
        std::string channelName = channels.back();
        Channel *channel = _findChannel(channelName);
        if (!channel)
        {
            sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + channelName + " :No such channel");
            channels.pop_back();
            continue;
        }
        if (!channel->CheckOperator(client))
        {
            sendMessage(NULL, client, ERR_CHANOPRIVSNEEDED, 0, " " + channelName + " :You're not channel operator");
            channels.pop_back();
            continue;
        }
        else
        {
            while (targets.size())
            {
                std::string target = targets.back();
                Client *targetClient = FindClientInChannel(target, channel);
                if (!targetClient)
                {
                    sendMessage(NULL, client, ERR_NOSUCHNICK, 0, " " + target + " :No such nick/channel");
                    targets.pop_back();
                    continue;
                }
                YeetMember(client, targetClient, channel, reason);
                targets.pop_back();
            }
        }
        channels.pop_back();
    }
}

void toLower(std::string &str)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        str[i] = std::tolower(str[i]);
    }
}

std::string removeColon(std::string topic)
{
    if (topic[0] == ':' && topic[1] == ':')
        topic = topic.substr(2, topic.size() - 1);
    else if (topic[0] == ':')
        topic = topic.substr(1, topic.size() - 1);
    return (topic);
}
std::string getTopic(std::vector<std::string> tokens)
{
    std::string topic;
    if (tokens.size() == 2)
        return ("");
    if (tokens[2][0] == ':')
    {
        for (size_t i = 2; i < tokens.size(); i++)
        {
            topic += tokens[i];
            if (i != tokens.size() - 1)
                topic += " ";
        }
    }
    else
        topic = tokens[2];
    topic = removeColon(topic);
    return (topic);
}

/// @brief the topic command is used to change or view the topic of a channel
/// @param client the client requesting the topic change
/// @param tokens contains the channel name and the new topic
void Server::_topicCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() == 2 && tokens[1] == ":")
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "TOPIC :Not enough parameters");
        return;
    }
    std::string topic = getTopic(tokens);
    Channel *channel = _findChannel(tokens[1]);
    if (!channel)
    {
        sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + tokens[1] + " :No such channel");
        return;
    }
    if (!channel->CheckMember(client))
    {
        sendMessage(NULL, client, ERR_NOTONCHANNEL, 0, " " + channel->getName() + " :You're not on that channel");
        return;
    }
    int token_flag = (tokens.size() > 2) ? 1 : 0;
    channel->setTopic(client, topic, token_flag);
}

Client *Server::findClientByNickname(const std::string &nickname)
{
    std::map<int, Client *>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second->getNickname() == nickname)
        {
            return it->second;
        }
    }
    return nullptr;
}

void Server::CheckMembership(Client *client, Channel *channel)
{
    if (!channel->CheckOperator(client))
    {
        sendMessage(NULL, client, ERR_CHANOPRIVSNEEDED, 0, channel->getName() + " :You're not channel operator");
        return;
    }
    if (!channel->CheckMember(client))
    {
        sendMessage(NULL, client, ERR_NOTONCHANNEL, 0, channel->getName() + " :You're not on that channel");
        return;
    }
}

void Server::_inviteCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 3)
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "INVITE :Not enough parameters");
        return;
    }
    std::string nickname = tokens[1];
    std::string channelName = tokens[2];
    Client *invitedClient = findClientByNickname(nickname);
    Channel *channel = _findChannel(channelName);
    if (!invitedClient)
    {
        sendMessage(NULL, client, ERR_NOSUCHNICK, 0, nickname + " :No such nickname");
        return;
    }
    if (!channel)
    {
        sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, channelName + " :No such channel");
        return;
    }
    CheckMembership(client, channel);
    channel->AddInvitedMember(client, invitedClient);
    theBootLegSendMessage(client, ":localhost 341 " + client->getNickname() + " " + invitedClient->getNickname() + " " + channel->getName() + "\r\n");
    theBootLegSendMessage(client, ":localhost NOTICE @" + channel->getName() + " :" + client->getNickname() + " invited " + nickname + " into channel " + channel->getName() + "\r\n");
    theBootLegSendMessage(invitedClient, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + " INVITE " + invitedClient->getNickname() + " " + channel->getName() + "\r\n");
}

std::vector<std::string> readLinesFromFile(const std::string &filename)
{
    std::vector<std::string> lines;
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::string line;
        while (std::getline(file, line))
            lines.push_back(line);
        file.close();
    }
    return lines;
}

std::string printCurrentTime()
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::ctime(&currentTime);
    return (timeString);
}

std::string FactGen()
{
    std::string filename = "obscureunsettlingfacts.txt";
    std::vector<std::string> lines = readLinesFromFile(filename);
    std::srand(std::time(0));
    int randomIndex = std::rand() % lines.size();
    return (lines[randomIndex]);
}

void Server::_botCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 2)
    {
        sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, "BOT :Not enough parameters, try BOT DATE or BOT FACT");
        return;
    }
    std::string command = tokens[1];
    if (command == "DATE")
    {
        std::string result = printCurrentTime();
        // sendMessage(NULL, client, 0, 0, result);
        theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + " NOTICE " + client->getNickname() + " :" + "The time is: " + result + "\r\n");
    }
    else if (command == "FACT")
    {
        std::string result = FactGen();
        // sendMessage(NULL, client, 0, 0, result);
        theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + " NOTICE " + client->getNickname() + " :" + "A fact that might be disturbing owo: " + result + "\r\n");
    }
    else if (command == "HELP")
        sendMessage(NULL, client, 0, 0, "BOT DATE: displays the current date and time\nBOT FACT: displays a random fact");
    else
        sendMessage(NULL, client, ERR_UNKNOWNCOMMAND, 0, "BOT :Unknown command, try BOT DATE or BOT FACT");
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

    std::map<std::string, CommandFunction>::iterator it = commandMap.find(command);
    if (it != commandMap.end())
    {
        CommandFunction func = it->second;
        (this->*func)(client, tokens);
    }
    else
    {
        // Unknown command
        sendMessage(NULL, client, ERR_UNKNOWNCOMMAND, 0, " " + command + " :Unknown command");
    }
}

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