#include "DataStore.h"
#include "ServerConfig.h"

#include "Poco/Path.h"
#include "Poco/FileStream.h"
#include "Poco/StringTokenizer.h"

DataStore<ItemStore> sItemStore;

template <typename S, int bufferSize>
DataStore<S, bufferSize>::DataStore()
{
}

template <typename S, int bufferSize>
DataStore<S, bufferSize>::~DataStore()
{
    while (!_dataStore.empty())
    {
        TypeDataStoreMap::iterator itr = _dataStore.begin();
        delete itr->second;
        _dataStore.erase(itr);
    }
}

// Read all the items
bool DataStore<ItemStore>::read(std::string filename)
{
    Poco::Path path(sConfig.getDefaultString("DataFolder", "data").append("/").append(filename));
    Poco::File store(path);

    if (!store.exists())
        return false;

    Poco::FileInputStream input(store.path());

    char buffer[bufferSize];
    while (input.getline(buffer, bufferSize).good())
    {
        Poco::StringTokenizer tokens(buffer, "\t");
        
        ItemStore* itemStore = new ItemStore();
        itemStore->Id = atoi(tokens[0].c_str());
        itemStore->Name = tokens[1];
        itemStore->Level = atoi(tokens[2].c_str());
        itemStore->Gender = atoi(tokens[3].c_str());

        _dataStore.insert(rde::make_pair(itemStore->Id, itemStore));
    }

    return true;
}
