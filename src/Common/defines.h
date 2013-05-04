#ifndef BASIC_SERVER_DEFINES_H
#define BASIC_SERVER_DEFINES_H

#include "Poco/Poco.h"

#define MAKE_GUID(a, b) ( ((Poco::UInt64)a << 32) | b )
#define LOGUID(a) Poco::UInt32( a & 0xFFFFFFFF )
#define HIGUID(a) Poco::UInt32( a >> 32 )
enum HIGH_GUID
{
    HIGH_GUID_PLAYER    = 0x0001,
    HIGH_GUID_CREATURE  = 0x0002,
    HIGH_GUID_ITEM      = 0x0004,
};
#define MAX_HIGH_GUID 3

#define MAX_X           256
#define MAX_Y           256
#define UNITS_PER_CELL  500

struct Vector2D
{
    Vector2D()
    {
    }

    Vector2D(float x, float y)
    {
        this->x = x;
        this->y = y;
    }

    float x;
    float y;
};

struct equint64
{
    bool operator() (const Poco::UInt64 u1, const Poco::UInt64 u2) const
    {
        return u1 == u2;
    }
};
    
#include <set>
typedef std::set<Poco::UInt64> GuidsSet;

//@ Testing purpouses macros
#define SERVER_FRAMEWORK_TESTING

#if defined(SERVER_FRAMEWORK_TESTING)
    #define SERVER_FRAMEWORK_TEST_SUITE
#endif

//@ Login database
class AuthDatabaseConnection;
extern AuthDatabaseConnection AuthDatabase;

//@ Characters database
class CharactersDatabaseConnection;
extern CharactersDatabaseConnection CharactersDatabase;

//@ Global Server Pointer (Object Accessing)
class Server;
extern Server* sServer;

#endif