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
#include "debugging.h"

//@ Basic server information
// >> Server runs on multiple threads, grids are in a thread pool
#include "Poco/ErrorHandler.h"
#include "Poco/ThreadPool.h"

//@ Everything is stored in SharedPtrs
#include "Poco/SharedPtr.h"

#ifdef SERVER_FRAMEWORK_TESTING

    #include "Object.h"
    #include "Tools.h"

    //@ Basic Net connections
    #include "Poco/Net/Net.h"
    #include "Poco/Net/ServerSocket.h"
    #include "Poco/Net/StreamSocket.h"

    using Poco::Net::Socket;
    using Poco::Net::StreamSocket;
    using Poco::Net::SocketAddress;

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

    // Databases
    bool dbAuth = AuthDatabase.Open("host=127.0.0.1;user=root;password=;port=3306;db=auth");
    bool dbChars = CharactersDatabase.Open("host=127.0.0.1;user=root;password=;port=3306;db=characters");

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
