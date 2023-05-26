#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>


void sendMsg(int fd, const std::string &msg)
{
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, msg.c_str());
    write(fd, buffer, sizeof(buffer));
}

std::string printCurrentTime() 
{
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::ctime(&currentTime);
    return (timeString);
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

std::string FactGen()
{
    std::string filename = "obscureunsettlingfacts.txt";
    std::vector<std::string> lines = readLinesFromFile(filename);
    std::srand(std::time(0));
    int randomIndex = std::rand() % lines.size();
    return (lines[randomIndex]);
}

int main()
{
    FactGen();
    printCurrentTime();
    return 0;
}
