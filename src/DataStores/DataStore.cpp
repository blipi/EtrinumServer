#include "DataStore.h"
#include "ServerConfig.h"

#include "Poco/Path.h"
#include "Poco/File.h"

DataStore::DataStore(std::string file)
{
	Poco::Path path(sConfig.getDefaultString("DataFolder").append("Items.ds"));
	Poco::File file(path);
	
	if (!file.exists())
		return;
		
	
}

