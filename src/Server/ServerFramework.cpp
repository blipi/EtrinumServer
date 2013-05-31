#include <stdio.h>

#include "AuthDatabase.h"
#include "CharactersDatabase.h"
#include "debugging.h"
#include "defines.h"
#include "GridLoader.h"
#include "Log.h"
#include "Server.h"
#include "ServerConfig.h"

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
    sServer = new Server();
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
    
    // --------------------- //
    // Initialize GridLoader //
    // --------------------- //
    {
        sLog.out(Message::PRIO_INFORMATION, "\n[*] Initializing GridLoader");
        sGridLoader.initialize(sServer);
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
        sGridLoader.addObject(plr);
        //MotionMaster::StartAngleMovement(plr, 0.5f, 500.5f);
        //MotionMaster::StartSimpleMovement(plr, Vector2D(400.0f, 200.0f), 15.0f);

    #endif

    sServer->start(1616);

    delete sServer;
}

int main(int argc, char** argv)
{
    // Read configuration file
    sLog.out(Message::PRIO_INFORMATION, "[*] Reading Configuration File");
    sConfig.readConfiguration();
        
    // Set log level
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Setting LogLevel to %d\n", sConfig.getIntConfig("LogLevel"));
    sLog.setLogLevel(Message::Priority(sConfig.getIntConfig("LogLevel")));

    // Initialize the Error Handler and MySQL
    MyErrorHandler eh;
    Poco::ErrorHandler* oldErrorHandler = Poco::ErrorHandler::set(&eh);    
    
    sLog.out(Message::PRIO_INFORMATION, "[*] Initializing MySQL");
    MySQL::Connector::registerConnector();
    
    // Read database configuration
    Config::StringConfigsMap connectionStrings = sConfig.getDatabaseInformation();

    // Databases
    AuthDatabase.Open(connectionStrings["auth"]);
    CharactersDatabase.Open(connectionStrings["characters"]);
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
        
    MySQL::Connector::unregisterConnector();
    Poco::ErrorHandler::set(oldErrorHandler);

    sLog.out(Message::PRIO_INFORMATION, "");
    system("pause");
    return 0;
}
