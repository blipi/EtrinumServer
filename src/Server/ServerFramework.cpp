#include <stdio.h>
#include <iostream>
#include <limits>
#include <map>
#include <list>

#include "Server.h"
#include "defines.h"
#include "Grid.h"
#include "AuthDatabase.h"
#include "CharactersDatabase.h"
#include "Config.h"
#include "Log.h"
#include "debugging.h"

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/ThreadPool.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

#ifdef SERVER_FRAMEWORK_TEST_SUITE

    #include "ObjectManager.h"
    #include "Object.h"
    #include "Player.h"
    #include "Tools.h"

#endif

using Poco::Thread;

Server* sServer = NULL;

AuthDatabaseConnection AuthDatabase;
CharactersDatabaseConnection CharactersDatabase;

class MyErrorHandler: public Poco::ErrorHandler
{
public:
    void exception(const Poco::Exception& exc)
    {
        sLog.out(Message::PRIO_CRITICAL, std::string("Poco Error triggered: ").append(exc.displayText()));
    }

    void exception(const std::exception& exc)
    {
        sLog.out(Message::PRIO_CRITICAL, std::string("STD Error triggered: ").append(exc.what()));
    }

    void exception()
    {
        sLog.out(Message::PRIO_CRITICAL, "Unknown Error");
    }
};

void doInitialize()
{
    sLog.out(Message::PRIO_INFORMATION, "\n[*] Initializing server");
    Server server(1616);
    sServer = &server;
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
    
    // --------------------- //
    // Initialize GridLoader //
    // --------------------- //
    {
        sLog.out(Message::PRIO_INFORMATION, "\n[*] Initializing GridLoader");
        sGridLoader.initialize(&server);
        sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
    }

    // --------------- //
    // Server is ready //
    // --------------- //
    {
        sLog.out(Message::PRIO_INFORMATION, "\n[OK] Server is running\n");
    }

    #ifdef SERVER_FRAMEWORK_TEST_SUITE

        SharedPtr<Player> plr = sObjectManager.createPlayer("ASD", NULL);
        SharedPtr<Object> obj = sObjectManager.create(HIGH_GUID_CREATURE);

        sGridLoader.addObject(plr)->addObject(obj);

        MotionMaster::StartAngleMovement(plr, 0.5f, 25.0f);
        MotionMaster::StartAngleMovement(obj, 0.5f, 25.0f);

    #endif

    while (server.isRunning())
    {
        // Parse CLI (Console Input)
        Thread::sleep(200);
    }
}

int main(int argc, char** argv)
{
    MyErrorHandler eh;
    Poco::ErrorHandler* oldErrorHandler = Poco::ErrorHandler::set(&eh);
    
    MySQL::Connector::registerConnector();
    
    sLog.out(Message::PRIO_INFORMATION, "[*] Initializing MySQL");

    // Read database configuration
    Config::DatabaseConnectionsMap connectionStrings = Config::readDatabaseInformation();

    // Databases
    bool dbAuth = AuthDatabase.Open(connectionStrings["auth"]);
    bool dbChars = CharactersDatabase.Open(connectionStrings["characters"]);

    if (!dbAuth || !dbChars)
        sLog.out(Message::PRIO_FATAL, "\t[Fail] Auth (%d) - Characters (%d)", dbAuth, dbChars);
    else
    {
        sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
        doInitialize();
    }
    
    MySQL::Connector::unregisterConnector();
    Poco::ErrorHandler::set(oldErrorHandler);

    printf("\n");
    system("pause");
    return 0;
}
