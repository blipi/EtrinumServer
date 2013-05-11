#include "CharactersDatabase.h"

CharactersDatabaseConnection::CharactersDatabaseConnection()
{
}

CharactersDatabaseConnection::~CharactersDatabaseConnection()
{
}

void CharactersDatabaseConnection::DoPreparedStatements()
{
    stmts.resize(MAX_CHARACTERSDATABASE_STATEMENTS);

    try
    {
	    DO_PREPARED_STATEMENT(QUERY_CHARACTERS_SELECT_BASIC_INFORMATION, "SELECT c.id, c.model, c.name FROM characters c WHERE c.account = ?")
	    DO_PREPARED_STATEMENT(QUERY_CHARACTERS_UPDATE_GUID, "UPDATE characters c SET c.guid = ? WHERE c.id = ?")
	    DO_PREPARED_STATEMENT(QUERY_CHARACTERS_SELECT_INFORMATION, "SELECT c.x, c.y, c.maxhp, c.hp, c.maxmp, c.mp, c.lvl FROM characters c WHERE c.id = ?")
        DO_PREPARED_STATEMENT(QUERY_CHARACTERS_SELECT_VISIBLE_EQUIPMENT, "SELECT i.item_id FROM inventory i WHERE i.pid = ? and i.slot <= 6")
    }
    catch (Poco::Exception e)
    {
        printf ("ERROR: %s\n", e.displayText().c_str());
    }
}
