#ifndef _SERVERFRAMEWORK_DATABASE_AUTH_H
#define _SERVERFRAMEWORK_DATABASE_AUTH_H

#include "Poco/Data/Column.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/MySQL/Connector.h"

#include <vector>

#include "Database.h"

using namespace Poco::Data;

enum AuthDatabaseStatements
{
    QUERY_AUTH_LOGIN,
	QUERY_AUTH_UPDATE_ONLINE_ONSTART,
	QUERY_AUTH_UPDATE_SID,
	QUERY_AUTH_UPDATE_ONLINE,
	QUERY_AUTH_GET_ONLINE,
	QUERY_AUTH_LOGIN_SID,

	MAX_AUTHDATABASE_STATEMENTS
};

class AuthDatabaseConnection : public Database
{
public:
    AuthDatabaseConnection();
    ~AuthDatabaseConnection();

    void DoPreparedStatements();
};

#endif