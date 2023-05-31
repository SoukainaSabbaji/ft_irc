#include "../includes/Server.hpp"

#include "../includes/Client.hpp"


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

bool ValidateNick(const std::string& nickname)
{
    if (!std::isalpha(nickname[0]) || nickname.empty() || nickname.size() > 9
    || nickname.find_first_of(" ,*?!@.") != std::string::npos ||
        nickname == "nick" || nickname == "NICK")
        return false;
    for (std::size_t i = 1; i < nickname.size(); ++i)
    {
        char ch = nickname[i];
        if (!std::isalnum(ch) && ch != '_' && ch != '-' && ch != '[' && ch != ']')
            return false;
    }
    return true;
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
    if (!ValidateNick(token))
    {
        sendMessage(NULL, client, ERR_ERRONEUSNICKNAME, 0, " :" + token + " :Erroneous nickname");
        return;
    }
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
    if (_channels.empty()) 
        return nullptr; 
    std::vector<Channel *>::const_iterator i;
    for (i = _channels.cbegin(); i != _channels.cend(); i++)
    {
        if ((*i)->getChannelName() == channelName)
            return (*i);
    }
    return nullptr;
}
bool Server::nickAvailable(std::string nickname) const
{
    if (_nicknames.find(nickname) == _nicknames.end())
        return (true);
    return (false);
}


void Server::BroadcastMessage(Client *client, Channel *target, const std::string &message)
{
    std::string senderNick = client->getNickname();
    std::string fullMessage = ":" + senderNick + " PRIVMSG " + target->getName() + " :" + message + "\r\n";

    std::vector<Client *> clients = target->getClients();
    for (size_t i = 0; i < clients.size(); i++)
    {
        Client *clientTarg = clients[i];
        if (clientTarg != client)
        {
            int bytesSent = send(clientTarg->getFd(), fullMessage.c_str(), fullMessage.length(), 0);
            if (bytesSent == -1)
            {
                std::cout << "Error sending message to client" << std::endl;
            }
        }
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
        if (target && target->CheckMember(client))
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
            if (_nbrchannels >= 20)
            {
                sendMessage(NULL, client, ERR_TOOMANYCHANNELS, 0, " " + channelName + " :The maximum number of channels has been reached");
                channels.pop_back();
                continue;
            }
            channel = new Channel(channelName, client);
            _channels.push_back(channel);
            channel->_server = this;
            _nbrchannels++;
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
    std::string StartMessage = ":irc.soukixie.local 321 " + client->getNickname() + "  :Start of /LIST command.\r\n";
    send(client->getFd(), StartMessage.c_str(), StartMessage.size(), 0);
    if (tokens.size() > 1)
        channels = SplitTargets(tokens[1]);
    else 
    {
        for (size_t i = 0; i < _channels.size(); i++)
            channels.push_back(_channels[i]->getName());
    }
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
        std::string message;
        std::stringstream ss;
        ss << ":irc.soukixie.local 322 " << client->getNickname() << " " << channel->getName() << ft_itoa(channel->getClients().size()) << " :";
        ss << channel->getTopic() << "\r\n";
        message = ss.str();
        send(client->getFd(), message.c_str(), message.size(), 0);

        channels.pop_back();
    }
    sendMessage(NULL, client, RPL_LISTEND, 0, " :End of /LIST");
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

void Server::DeleteEmptyChan(Channel* channel, std::vector<Channel*>& _channels)
{
    // check if the channel is empty, if so delete it
    if (channel->getClients().size() == 0)
    {
        std::vector<Channel*>::iterator it = std::find(_channels.begin(), _channels.end(), channel);
        if (it != _channels.end())
        {
            _channels.erase(it);
            delete channel;
        }
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
    std::cout << _channels.size() << std::endl;
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
    std::string reason = getReason(tokens);
    // check if the client requesting the kick is an operator in the channel
    for (std::vector<std::string>::reverse_iterator it = channels.rbegin(); it != channels.rend(); ++it)
    {
        std::string channelName = *it;
        Channel *channel = _findChannel(channelName);
        if (!channel)
        {
            sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " " + channelName + " :No such channel");
            continue;
        }
        if (!channel->CheckOperator(client))
        {
            sendMessage(NULL, client, ERR_CHANOPRIVSNEEDED, 0, " " + channelName + " :You're not channel operator");
            continue;
        }
        else
        {
            std::vector<std::string> channelTargets = SplitTargets(tokens[2]);
            for (std::vector<std::string>::reverse_iterator targetIt = channelTargets.rbegin(); targetIt != channelTargets.rend(); ++targetIt)
            {
                std::string target = *targetIt;
                Client *targetClient = FindClientInChannel(target, channel);
                if (!targetClient)
                {
                    sendMessage(NULL, client, ERR_NOSUCHNICK, 0, " " + target + " :No such nick/channel");
                    continue;
                }
                YeetMember(client, targetClient, channel, reason);
            }
        }
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
        theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + " NOTICE " + client->getNickname() + " :" + "The time is: " + result + "\r\n");
    }
    else if (command == "FACT")
    {
        std::string result = FactGen();
        theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + " NOTICE " + client->getNickname() + " :" + "A fact that might be disturbing owo: " + result + "\r\n");
    }
    else if (command == "HELP")
        sendMessage(NULL, client, 0, 0, "BOT DATE: displays the current date and time\nBOT FACT: displays a random fact");
    else
        sendMessage(NULL, client, ERR_UNKNOWNCOMMAND, 0, "BOT :Unknown command, try BOT DATE or BOT FACT");
}

void Server::_pingCommand(Client *client, std::vector<std::string> tokens)
{
    CheckAuthentication(client);
    if (tokens.size() < 2)
    {
        sendMessage(NULL, client, ERR_NOORIGIN, 0, "PING :No origin specified");
        return;
    }
    std::string origin = tokens[1];
    std::string message = ":" + this->_serverName + " PONG " + origin + "\r\n";
    send(client->getFd(), message.c_str(), message.size(), 0);
}

void	Server::_quitCommand(Client *clt, std::vector<std::string> tokens)
{
	Client *client;
	size_t	len;

	if (tokens[0] == "quit")
	{
		len = _channels.size();
		for (size_t i = 0; i < len; ++i)
		{
			client = FindClientInChannel(clt->getNickname(),_channels[i]);
			if (client)
			{
				_channels[i]->TheBootlegBroadcast(":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" + + " QUIT " + ":Quit: client is a quitter booooo!!!");
				_channels[i]->removeClient(client, "Quit: client is a quitter booo!!!");
			}
		}
		std::cout<<clt->getFd()<<std::endl;
		theBootLegSendMessage(clt, "ERROR : Closing Link: "+ clt->getNickname() + "[Quit: client is a quitter booo!!]\r\n");
		this->removeClient(clt->getFd());
	}
}

/*TODO for sixie check if mode is already and send an error if it's already set*/

void Server::applyAddForAllChannels(Client *client, std::vector<std::string> chnls, short mode, std::string param)
{
	std::vector<std::string> targets;
	std::vector<std::string> ops;
	Channel *chnl;

	targets = SplitTargets(param);
	for (size_t i = 0; i < chnls.size() ;++i)
	{
		chnl = _findChannel(chnls[i]);
		if (!chnl)
		{
			sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " :no such channel " + chnls[i]);
			continue ;
		}
		if (!chnl->isOnChannel(client))
		{
			sendMessage(NULL, client, ERR_USERNOTINCHANNEL, 0, " MODE: user not on channel "+ chnl->getChannelName());
			continue;
		}
		ops = chnl->getOperators();
		if (!std::count(ops.begin(), ops.end(), client->getNickname()))
		{
			sendMessage(NULL, client, ERR_NOPRIVILEGES, 0, " MODE: Not operator on " + chnl->getChannelName());
			continue;
		}
		if (mode == 3)
		{
			size_t j = 0;
			while (j < param.size())
			{
				if (!std::isdigit(param[j]))
					break ;
				++j;
			}
			if (j != param.size())
			{
				theBootLegSendMessage(client, "ERROR : Bad Args: "+ client->getNickname() + "[MODE: Bad args]\r\n");
				continue;
			}
			chnl->setLimit(std::atoi(param.c_str()));
			chnl->setMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " +l " + param);
		}
		else if (mode == 4)
		{
			for (size_t j = 0; j < targets.size(); ++j)
			{
				if (!findClientByNickname(targets[j]))
					sendMessage(NULL, client, ERR_NOSUCHNICK, 0, " MODE: no such nickname "+ targets[j]);
				else
				{
					chnl->setOperator(findClientByNickname(targets[j]));
					theBootLegSendMessage(client,":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " +o" + targets[j]);
				}
			}
		}
		else if (mode == 2)
		{
			chnl->setKey(param);
			chnl->setMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " +k " + param);
		}
		else if (mode == 1)
		{
			chnl->setTopic(client, param, 1); //TODO: souki check if this is the right way still not sure 
			chnl->setMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " +t " + param);
		}
		else if (mode == 0)
		{
			chnl->setMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " +i");
		}
	}
}

void Server::applyRmForAllChannels(Client *client, std::vector<std::string> chnls, short mode, std::string param)
{
	std::vector<std::string> targets;
	std::vector<std::string> ops;
	Channel *chnl;

	targets = SplitTargets(param);
	for (size_t i = 0; i < chnls.size() ;++i)
	{
		chnl = _findChannel(chnls[i]);
		if (!chnl)
		{
			sendMessage(NULL, client, ERR_NOSUCHCHANNEL, 0, " :no such channel " + chnls[i]);
			continue ;
		}
		if (!chnl->isOnChannel(client))
		{
			sendMessage(NULL, client, ERR_USERNOTINCHANNEL, 0, " MODE: user not on channel "+ chnl->getChannelName());
			continue;
		}
		ops = chnl->getOperators();
		if (!std::count(ops.begin(), ops.end(), client->getNickname()))
		{
			sendMessage(NULL, client, ERR_NOPRIVILEGES, 0, " MODE: Not operator on " + chnl->getChannelName());
			continue;
		}
		if (mode == 4)
		{
			for (size_t j = 0; j < targets.size(); ++j)
			{
				if (!findClientByNickname(targets[j]))
					sendMessage(NULL, client, ERR_NOSUCHNICK, 0, " MODE: no such nickname "+ targets[j]);
				else
				{
					chnl->removeOperator(findClientByNickname(targets[j]));
					theBootLegSendMessage(client,":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " -o" + targets[j]);
				}
			}
		}
		else if (mode == 3)
		{
			chnl->setLimit(0);
			chnl->removeMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " -l ");
		}
		else if (mode == 2)
			continue ;
		else if (mode == 1)
		{
			chnl->removeMode(mode);
			// chnl->removeTopic(); //TODO: souki implement this function idk how topic really works
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " -t ");
		}
		else if (mode == 0)
		{
			chnl->removeMode(mode);
			theBootLegSendMessage(client, ":" + client->getNickname() + "!~" + client->getUsername() + "@irc.soukixie.local" +  "MODE :" + chnl->getChannelName() + " -i ");
		}
	}
}

void Server::_modeCommand(Client *client, std::vector<std::string> tokens)
{
	std::vector<std::string> channels;
	std::string knownArgs = "+-itklosn";
	size_t	holder = 0;

	CheckAuthentication(client);
	if (tokens.size() < 3)
		return (sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: need more params"));
	channels = SplitTargets(tokens[1]);
	for (size_t i = 2; i < tokens.size(); ++i)
	{
		for (size_t j = 0; j < tokens[i].size(); ++j)
		{
			if (knownArgs.find(tokens[i][j]) != std::string::npos)
			{
				while (tokens[i][j] == '+' || tokens[i][j] == '-')
					++j;
				if (tokens[i][j] == 'o' && (!j || tokens[i][j - 1] != '-'))
				{
					if ((i + (++holder)) < tokens.size())
						applyAddForAllChannels(client, channels, 4, tokens[i + holder]); //operator
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ; // continuing will break the whole function's flow so i just return
					}
				}
				else if (tokens[i][j] == 'o' && (!j || tokens[i][j - 1] == '-'))
					if (i + (++holder) < tokens.size())
						applyRmForAllChannels(client, channels, 4, tokens[i + holder]);
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ; // continuing will break the whole function's flow so i just return
					}
				else if (tokens[i][j] == 'l' && (!j || tokens[i][j - 1] != '-'))
					if (i + (++holder) < tokens.size())
						applyAddForAllChannels(client, channels, 3, tokens[i + holder]);
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ;
					}
				else if (tokens[i][j] == 'l' && (!j || tokens[i][j - 1] == '-'))
					applyRmForAllChannels(client, channels, 3, "");
				else if (tokens[i][j] == 'k')
					if (i + (++holder) < tokens.size())
						applyAddForAllChannels(client, channels, 2, tokens[i + holder]);
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ;
					}
				else if (tokens[i][j] == 't' && (!j || tokens[i][j - 1] != '-'))
					if (i + (++holder) < tokens.size())
						applyAddForAllChannels(client, channels, 1, tokens[i + holder]);
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ;
					}
				else if (tokens[i][j] == 't' && (!j || tokens[i][j - 1] == '-'))
					applyRmForAllChannels(client, channels, 1, "");
				else if (tokens[i][j] == 'i' && (!j || tokens[i][j - 1] != '-'))
					if (i + (++holder) < tokens.size())
						applyAddForAllChannels(client, channels, 0, tokens[i + holder]);
					else
					{
						sendMessage(NULL, client, ERR_NEEDMOREPARAMS, 0, " MODE: needs more params");
						return ;
					}
				else if (tokens[i][j] == 'i' && (!j || tokens[i][j - 1] == '-'))
					applyRmForAllChannels(client, channels, 0, "");
			}
			else
				theBootLegSendMessage(client, "ERROR : Bad Args: "+ client->getNickname() + "[MODE: Bad args]\r\n");
		}
		i += holder;
		holder = 0;
	}
}
