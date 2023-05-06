#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

const int PORT = 6969;

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
        std::cout << "Client: " << buffer << std::endl;

        memset(buffer, 0, sizeof(buffer));
        std::cin.getline(buffer, sizeof(buffer));
        write(client_sfd, buffer, sizeof(buffer));
    }

    close(server_sfd);
    return 0;
}