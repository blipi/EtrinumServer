#ifndef GAMESERVER_DATASTORE_H
#define GAMESERVER_DATASTORE_H

#include "Poco/Poco.h"
#include "Poco/File.h"

//@ List and Hash Map
#include "hash_map.h"
#include "stack_allocator.h"

#include <string>

struct ItemStore
{
    Poco::UInt32 Id;
    std::string Name;
    Poco::UInt8 Level;
    Poco::UInt8 Gender;
};

template <typename S, int bufferSize = 1024>
class DataStore
{
public:
    DataStore();
    ~DataStore();

    bool read(std::string file);

private:
    typedef rde::hash_map<Poco::UInt32 /*id*/, S*> TypeDataStoreMap;    
    TypeDataStoreMap _dataStore;
};

extern DataStore<ItemStore> sItemStore;

#endif