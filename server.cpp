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
#define BUFFER_SIZE 1024

const int PORT = 6968;

void sendMsg(int fd, const std::string &msg)
{
    //     char buffer[1024];
    //     memset(buffer, 0, sizeof(buffer));
    //     strcpy(buffer, msg.c_str());
    //     write(fd, buffer, sizeof(buffer));
    int ret = send(fd, msg.c_str(), msg.length(), 0);
    if (ret == -1 && (errno == EPIPE || errno == ECONNRESET))
    {
        close(fd);
        fd = -1;
    }
}

void printCurrentTime(int fd)
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::ctime(&currentTime);
    sendMsg(fd, timeString);
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

void FactGen(int fd)
{
    std::string filename = "obscureunsettlingfacts.txt";
    std::vector<std::string> lines = readLinesFromFile(filename);
    std::srand(std::time(0));
    int randomIndex = std::rand() % lines.size();
    sendMsg(fd, lines[randomIndex]);
}

int main()
{
    int server_sfd, client_sfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    char buffer[1024];

    client_addr_len = sizeof(client_addr);

    server_sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sfd < 0)
    {
        std::cout << "Error creating socket" << std::endl;
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "Error binding socket" << std::endl;
        return -1;
    }

    if (listen(server_sfd, 5) < 0)
    {
        std::cout << "Error listening socket" << std::endl;
        return -1;
    }

    while (true)
    {
        client_sfd = accept(server_sfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sfd < 0)
        {
            std::cout << "Error accepting client" << std::endl;
            return -1;
        }

        memset(buffer, 0, sizeof(buffer));
        read(client_sfd, buffer, sizeof(buffer));
        if (strcmp(buffer, "fact") == 0)
        {
            std::cout << "FactGen" << std::endl;
            FactGen(server_sfd);
        }
        else if (strcmp(buffer, "time") == 0)
            printCurrentTime(client_sfd);
    }

    close(server_sfd);
    return 0;
}