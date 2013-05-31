#include "Database.h"

using namespace Poco::Data;
using namespace Poco::Data::Keywords;

PreparedStatement::PreparedStatement(Statement stmt):
    _stmt(stmt)
{
    std::string query = _stmt.toString();
    data.resize(std::count(query.begin(), query.end(), '?'));

    for (Poco::UInt8 i = 0; i < data.size(); i++)
        _stmt, use(data[i]);
}
    
void PreparedStatement::bindString(Poco::UInt8 pos, std::string str)
{
    data[pos].dataType = DATA_STRING;
    data[pos].str = str;
}

void PreparedStatement::bindUInt8(Poco::UInt8 pos, Poco::UInt8 value)
{
    data[pos].dataType = DATA_UINT8;
    data[pos].u8 = value;
}

void PreparedStatement::bindUInt16(Poco::UInt8 pos, Poco::UInt16 value)
{
    data[pos].dataType = DATA_UINT16;
    data[pos].u16 = value;
}

void PreparedStatement::bindUInt32(Poco::UInt8 pos, Poco::UInt32 value)
{
    data[pos].dataType = DATA_UINT32;
    data[pos].u32 = value;
}

void PreparedStatement::bindUInt64(Poco::UInt8 pos, Poco::UInt64 value)
{
    data[pos].dataType = DATA_UINT64;
    data[pos].u64 = value;
}

void PreparedStatement::bindInt8(Poco::UInt8 pos, Poco::Int8 value)
{
    data[pos].dataType = DATA_INT8;
    data[pos].i8 = value;
}

void PreparedStatement::bindInt16(Poco::UInt8 pos, Poco::Int16 value)
{
    data[pos].dataType = DATA_INT16;
    data[pos].i16 = value;
}

void PreparedStatement::bindInt32(Poco::UInt8 pos, Poco::Int32 value)
{
    data[pos].dataType = DATA_INT32;
    data[pos].i32 = value;
}

void PreparedStatement::bindInt64(Poco::UInt8 pos, Poco::Int64 value)
{
    data[pos].dataType = DATA_INT64;
    data[pos].i64 = value;
}

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
