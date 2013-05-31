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
#define UNITS_PER_CELL  100

#define MAP_MIN_X       -637
#define MAP_MAX_X       2837
#define MAP_MIN_Z       -816
#define MAP_MAX_Z       2000

struct Vector2D
{
    Vector2D()
    {
    }

    Vector2D(float x, float z)
    {
        this->x = x;
        this->z = z;
    }

    float x;
    float z;
};

//@ Avoid MSV defining max/min and overriding numeric_limits
#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
#endif
    
#include <set>
typedef std::set<Poco::UInt64> GuidsSet;

//@ Testing purpouses macros
#define SERVER_FRAMEWORK_TEST_SUITE

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