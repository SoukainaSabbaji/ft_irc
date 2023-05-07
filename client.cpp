// #include <iostream>
// #include <cstring>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>

// const int PORT = 6969;
// const char *HOST = "0.0.0.0";

// int main()
// {
//     int client_sfd;
//     struct sockaddr_in server_addr;
//     char buffer[1024];

//     client_sfd = socket(AF_INET, SOCK_STREAM, 0);
//     if (client_sfd < 0)
//     {
//         std::cout << "Error creating socket" << std::endl;
//         return -1;
//     }
//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT);
//     if (connect(client_sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
//     {
//         std::cout << "Error connecting to server" << std::endl;
//         return -1;
//     }

//     std::cout << "Connected to server" << std::endl;
//     std::cout << "Enter a message to send" << std::endl;

//     std::cin.getline(buffer, sizeof(buffer));
//     memset(buffer, 0, sizeof(buffer));
//     if (read(client_sfd, buffer, sizeof(buffer)) < 0)
//     {
//         std::cout << "Error receiving response" << std::endl;
//         return -1;
//     }
//     if (buffer[0] != '\0')
//     {
//         std::cout << "Server: " << buffer << std::endl;
//         close(client_sfd);
//         return 0;
//     }
// }

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

const int PORT = 6968;
const char *HOST = "0.0.0.0";

int main()
{
    int client_sfd;
    struct sockaddr_in server_addr;
    char buffer[1024];

    client_sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sfd < 0)
    {
        std::cout << "Error creating socket" << std::endl;
        return -1;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (connect(client_sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cout << "Error connecting to server" << std::endl;
        return -1;
    }

    std::cout << "Connected to server" << std::endl;
    std::cout << "Enter a message to send" << std::endl;

    std::cin.getline(buffer, sizeof(buffer));
    if (write(client_sfd, buffer, sizeof(buffer)) < 0)
    {
        std::cout << "Error sending message" << std::endl;
        return -1;
    }
    close(client_sfd);
    return 0;
}