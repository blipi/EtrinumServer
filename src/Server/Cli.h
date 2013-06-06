#ifndef GAMESERVER_CLI_H
#define GAMESERVER_CLI_H

#include <iostream>
#include <string>
#include <string>

class CLI
{
public:
    CLI();
    
    bool getCLI();
    
private:
    bool parseCLI(std::string cmd);
};

#endif