#ifndef BASIC_SERVER_PULLER_H
#define BASIC_SERVER_PULLER_H

//@ Avoid MSV defining max/min and overriding numeric_limits
#if defined(_WIN32) || defined(_WIN64)
    #define NOMINMAX
#endif

#include <list>

#include "Poco/Poco.h"
#include "Poco/RWLock.h"
#include "Poco/SingletonHolder.h"

#include "defines.h"
#include "google/dense_hash_map"

class Object;
class Client;

class ObjectManager
{
public:
    ObjectManager();
    static ObjectManager& instance()
    {
        static Poco::SingletonHolder<ObjectManager> sh;
        return *sh.get();
    }

    Object* create(Poco::UInt32 highGUID);
    Object* createPlayer(std::string name, Client* client);

    Object* getObject(Poco::UInt64 GUID);
    void removeObject(Poco::UInt64 GUID);

private:
    Poco::UInt32 newGUID(Poco::UInt32 highGUID);

public:
    static Poco::UInt32 MAX_GUID;

private:
    struct eqObj
    {
        bool operator() (Poco::UInt32 u1, Poco::UInt32 u2)
        {
            return u1 == u2;
        }
    };
    typedef google::dense_hash_map<Poco::UInt32 /*loguid*/, Object* /*object*/, std::hash<Poco::UInt32>, eqObj> ObjectsMap;
    typedef std::pair<Poco::UInt32 /*loguid*/, Object* /*object*/> ObjectInserter;

    ObjectsMap _players;
    ObjectsMap _creatures;
    ObjectsMap _items;

    Poco::UInt32 _playersGUID;
    Poco::UInt32 _creaturesGUID;
    Poco::UInt32 _itemsGUID;
};

#define sObjectManager ObjectManager::instance()

#endif