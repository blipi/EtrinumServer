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
#include "debugging.h"

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/ThreadPool.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

#ifdef SERVER_FRAMEWORK_TESTING

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
        std::cerr << "Poco Error triggered: " << exc.displayText() << std::endl;
    }

    void exception(const std::exception& exc)
    {
        std::cerr << "STD Error triggered: " << exc.what() << std::endl;
    }

    void exception()
    {
        std::cerr << "Unknown error" << std::endl;
    }
};

void doInitialize()
{
    printf("\n[*] Initializing server\n");
    Server server(1616);
    sServer = &server;
    printf("\t[OK] Done\n");
    
    // --------------------- //
    // Initialize GridLoader //
    // --------------------- //
    {
        printf("\n[*] Initializing GridLoader\n");
        sGridLoader.initialize(&server);
        printf("\t[OK] Done\n");
    }

    // --------------- //
    // Server is ready //
    // --------------- //
    {
        printf("\n[OK] Server is running\n\n");
    }

    #ifdef SERVER_FRAMEWORK_TESTING

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
    
    printf("[*] Initializing MySQL\n");

    // Read database configuration
    Config::DatabaseConnectionsMap connectionStrings = Config::readDatabaseInformation();

    // Databases
    bool dbAuth = AuthDatabase.Open(connectionStrings["auth"]);
    bool dbChars = CharactersDatabase.Open(connectionStrings["characters"]);

    if (!dbAuth || !dbChars)
        printf("\t[Fail] Auth (%d) - Characters (%d)\n", dbAuth, dbChars);
    else
    {
        printf("\t[OK] Done\n");
        doInitialize();
    }
    
    MySQL::Connector::unregisterConnector();
    Poco::ErrorHandler::set(oldErrorHandler);

    printf("\n");
    system("pause");
    return 0;
}
