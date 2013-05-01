#include "AuthDatabase.h"

AuthDatabaseConnection::AuthDatabaseConnection():
    Database()
{
}

AuthDatabaseConnection::~AuthDatabaseConnection()
{
}

void AuthDatabaseConnection::DoPreparedStatements()
{
    stmts.resize(MAX_AUTHDATABASE_STATEMENTS);

    try
    {
        DO_PREPARED_STATEMENT(QUERY_AUTH_LOGIN, "SELECT u.id, u.online FROM account u WHERE u.username = ? and u.password = ?");
        DO_PREPARED_STATEMENT(QUERY_AUTH_UPDATE_ONLINE_ONSTART, "UPDATE account u SET u.online = 0");
		DO_PREPARED_STATEMENT(QUERY_AUTH_UPDATE_SID, "UPDATE account u SET u.sid = ? WHERE u.id = ?")
		DO_PREPARED_STATEMENT(QUERY_AUTH_UPDATE_ONLINE, "UPDATE account u SET u.online = ? WHERE u.id = ?")
		DO_PREPARED_STATEMENT(QUERY_AUTH_GET_ONLINE, "SELECT u.online FROM account u WHERE u.id = ?")
		DO_PREPARED_STATEMENT(QUERY_AUTH_LOGIN_SID, "SELECT u.id FROM account u WHERE u.username = ? and u.password = ? and u.sid = ?")
    }
    catch (Poco::Exception e)
    {
        printf ("ERROR: %s\n", e.displayText().c_str());
    }
}
