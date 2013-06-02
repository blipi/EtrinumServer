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

#ifdef SERVER_FRAMEWORK_TEST_SUITE
    class Spawner : public Poco::Runnable
    {
    public:
        Spawner()
        {}

        ~Spawner()
        {}

        void run()
        {
            Thread::sleep(5000);

            int spawnLimit = 2000;
            int playerMax = 1000;
            float x = 0;
            float z = 0;

            for (int i = 0; i < spawnLimit; i++)
            {
                if ((i % 3 != 0) && playerMax > 0)
                {
                    SharedPtr<Player> plr = sObjectManager.createPlayer("ASD", NULL);
                    plr->Relocate(Vector2D(x, z));
                    sGridLoader.addObject(plr);
                    MotionMaster::StartSimpleMovement(plr, Vector2D(2800, 1000), SPEED_RUN);
                    playerMax--;
                
                    x += MAP_MAX_X / spawnLimit;
                    z += MAP_MAX_Z / spawnLimit;
                }
                else
                {
                    SharedPtr<Object> obj = sObjectManager.create(HIGH_GUID_CREATURE);
                    obj->Relocate(Vector2D(x, z));
                    sGridLoader.addObject(obj);
                    MotionMaster::StartSimpleMovement(obj, Vector2D(2800, 1000), SPEED_WALK);
                }

                Thread::sleep(10);
            }
        }
    };
#endif

int main(int argc, char** argv)
{
    // Read configuration file
    sLog.out(Message::PRIO_INFORMATION, "[*] Reading Configuration File");
    sConfig.readConfiguration();
        
    // Set log level
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Setting LogLevel to %d\n", sConfig.getDefaultInt("LogLevel", 4));
    sLog.setLogLevel(Message::Priority(sConfig.getDefaultInt("LogLevel", 4)));

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

    // Initialize the server
    sLog.out(Message::PRIO_INFORMATION, "\n[*] Initializing server");
    sServer = new Server();
    sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
    
    // --------------------- //
    // Initialize GridLoader //
    // --------------------- //
    {
        sLog.out(Message::PRIO_INFORMATION, "\n[*] Initializing Grid System");
        sGridLoader.instance();
        sLog.out(Message::PRIO_INFORMATION, "\t[OK] Done");
    }

    #ifdef SERVER_FRAMEWORK_TEST_SUITE
    
        Poco::Thread spawnerThread;
        Spawner spawner;
        spawnerThread.start(spawner);
        
    #endif

    // Will wait until the server stops
    sServer->start(sConfig.getDefaultInt("ServerPort", 1616));

    delete sServer;

    MySQL::Connector::unregisterConnector();
    Poco::ErrorHandler::set(oldErrorHandler);

    system("pause");
    return 0;
}
