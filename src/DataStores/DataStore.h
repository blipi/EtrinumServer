#ifndef GAMESERVER_DATASTORE_H
#define GAMESERVER_DATASTORE_H

//@ List and Hash Map
#include "hash_map.h"
#include "stack_allocator.h"

#include <string>

struct ItemStore
{
	Poco::UInt32 Id;
	std::string Name;
	Poco::UInt8 Level;
	Poco::Uint8 Gender;
};

template <typename S>
class DataStore
{
	DataStore(std::string file);
	~DataStore();

private:
	typedef rde::hash_map<Poco::UInt32 /*id*/, S*> TypeDataStoreMap;
	
	TypeDataStoreMap _dataStore;
};

#endif