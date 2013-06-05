#include "Cli.h"
#include "defines.h"
#include "Log.h"
#include "Server.h"

CLI::CLI()
{
}

bool CLI::getCLI()
{
    std::string cmd;
    getline(std::cin, cmd);
    
    return parseCLI(cmd);
}

bool CLI::parseCLI(std::string cmd)
{
    if (cmd.compare("diff") == 0)
        sLog.out(Message::PRIO_INFORMATION, "Server diff time: %d", sServer->getDiff());
    else if (cmd.compare("stop") == 0)
        return false;

    return true;
}
