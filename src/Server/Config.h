#ifndef _SERVERFRAMEWORK_CONFIG_H
#define _SERVERFRAMEWORK_CONFIG_H

#include <map>
#include <string>

namespace Config
{
    typedef std::map<std::string, std::string> DatabaseConnectionsMap;
    DatabaseConnectionsMap readDatabaseInformation();
}

#endif