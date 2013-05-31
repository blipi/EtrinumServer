#include "Database.h"

using namespace Poco::Data;

Database::Database()
{
}

Database::~Database()
{
}

void Database::Open(std::string connectionString)
{
    try
    {
        _pool = new SessionPool("MySQL", connectionString, 1, 32);
        if (!_pool->get().isConnected())
           ASSERT(false);
    }
    catch (Poco::Data::ConnectionFailedException ex)
    {
        printf("%s\n", ex.message().c_str());
        ASSERT(false);
    }
    catch (Poco::Data::MySQL::ConnectionException ex)
    {
        printf("%s\n", ex.message().c_str());
        ASSERT(false);
    }

    DoPreparedStatements();
}

PreparedStatement* Database::getPreparedStatement(Poco::UInt8 index)
{
    return stmts[index];
}
