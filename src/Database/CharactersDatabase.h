#ifndef GAMESERVER_DATABASE_CHARACTERS_H
#define GAMESERVER_DATABASE_CHARACTERS_H

#include "Poco/Data/Column.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/MySQL/Connector.h"

#include <vector>

#include "Database.h"

using namespace Poco::Data;

enum CharactersDatabaseStatements
{
    QUERY_CHARACTERS_SELECT_BASIC_INFORMATION,
    QUERY_CHARACTERS_UPDATE_GUID,
    QUERY_CHARACTERS_SELECT_INFORMATION,
    QUERY_CHARACTERS_SELECT_VISIBLE_EQUIPMENT,

	MAX_CHARACTERSDATABASE_STATEMENTS
};

class CharactersDatabaseConnection : public Database
{
public:
    CharactersDatabaseConnection();
    ~CharactersDatabaseConnection();

    void DoPreparedStatements();
};

#endif