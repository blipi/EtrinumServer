#ifndef _SERVERFRAMEWORK_DATABASE_H
#define _SERVERFRAMEWORK_DATABASE_H

#include "Poco/Poco.h"
#include "Poco/Data/Column.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"

#include "debugging.h"

using namespace Poco::Data;

#define DO_PREPARED_STATEMENT(a, b) stmts[a] = new PreparedStatement( (_pool->get() << b) );

enum PreparedStatementDataType
{
    DATA_STRING,
    DATA_UINT8,
    DATA_UINT16,
    DATA_UINT32,
    DATA_UINT64,
    DATA_INT8,
    DATA_INT16,
    DATA_INT32,
    DATA_INT64,
};

class PreparedStatementData
{
public:
    PreparedStatementData():
        str()
    {
        u8 = i8 = 0;
        u16 = i16 = 0;
        u32 = i32 = 0;
        u64 = i64 = 0;
    }

    PreparedStatementDataType dataType;
    
    std::string str;
    Poco::UInt8 u8;
    Poco::UInt16 u16;
    Poco::UInt32 u32;
    Poco::UInt64 u64;
    Poco::Int8 i8;
    Poco::Int16 i16;
    Poco::Int32 i32;
    Poco::Int64 i64;
};

namespace Poco {
namespace Data {

template <>
class TypeHandler<PreparedStatementData>
{
public:

    static std::size_t size()
    {
        return 1;
    }

    static void bind(std::size_t pos, const PreparedStatementData& obj, AbstractBinder* pBinder)
    {
        poco_assert_dbg (pBinder != 0);

        switch (obj.dataType)
        {
            case DATA_STRING:   TypeHandler<std::string>::bind(pos++, obj.str, pBinder);    break;
            case DATA_UINT8:    TypeHandler<Poco::UInt8>::bind(pos++, obj.u8, pBinder);     break;
            case DATA_UINT16:   TypeHandler<Poco::UInt16>::bind(pos++, obj.u16, pBinder);   break;
            case DATA_UINT32:   TypeHandler<Poco::UInt32>::bind(pos++, obj.u32, pBinder);   break;
            case DATA_UINT64:   TypeHandler<Poco::UInt64>::bind(pos++, obj.u64, pBinder);   break;
            case DATA_INT8:     TypeHandler<Poco::Int8>::bind(pos++, obj.i8, pBinder);      break;
            case DATA_INT16:    TypeHandler<Poco::Int16>::bind(pos++, obj.i16, pBinder);    break;
            case DATA_INT32:    TypeHandler<Poco::Int32>::bind(pos++, obj.i32, pBinder);    break;
            case DATA_INT64:    TypeHandler<Poco::Int64>::bind(pos++, obj.i64, pBinder);    break;
        }
    }

    static void prepare(std::size_t pos, const PreparedStatementData& obj, AbstractPreparation* pPrepare)
    {
        poco_assert_dbg (pPrepare != 0);

        switch (obj.dataType)
        {
            case DATA_STRING:   TypeHandler<std::string>::prepare(pos++, obj.str, pPrepare);    break;
            case DATA_UINT8:    TypeHandler<Poco::UInt8>::prepare(pos++, obj.u8, pPrepare);     break;
            case DATA_UINT16:   TypeHandler<Poco::UInt16>::prepare(pos++, obj.u16, pPrepare);   break;
            case DATA_UINT32:   TypeHandler<Poco::UInt32>::prepare(pos++, obj.u32, pPrepare);   break;
            case DATA_UINT64:   TypeHandler<Poco::UInt64>::prepare(pos++, obj.u64, pPrepare);   break;
            case DATA_INT8:     TypeHandler<Poco::Int8>::prepare(pos++, obj.i8, pPrepare);      break;
            case DATA_INT16:    TypeHandler<Poco::Int16>::prepare(pos++, obj.i16, pPrepare);    break;
            case DATA_INT32:    TypeHandler<Poco::Int32>::prepare(pos++, obj.i32, pPrepare);    break;
            case DATA_INT64:    TypeHandler<Poco::Int64>::prepare(pos++, obj.i64, pPrepare);    break;
        }
    }
};

}
}

class PreparedStatement
{
public:
    PreparedStatement(Statement stmt);

    void bindString(Poco::UInt8 pos, std::string str);
    void bindUInt8(Poco::UInt8 pos, Poco::UInt8 value);
    void bindUInt16(Poco::UInt8 pos, Poco::UInt16 value);
    void bindUInt32(Poco::UInt8 pos, Poco::UInt32 value);
    void bindUInt64(Poco::UInt8 pos, Poco::UInt64 value);
    void bindInt8(Poco::UInt8 pos, Poco::Int8 value);
    void bindInt16(Poco::UInt8 pos, Poco::Int16 value);
    void bindInt32(Poco::UInt8 pos, Poco::Int32 value);
    void bindInt64(Poco::UInt8 pos, Poco::Int64 value);
    
    inline RecordSet execute()
    {
        while (true)
        {
            try
            {
                _stmt.execute();
                if (_stmt.done())
                    return RecordSet(_stmt);
            }
            catch (Poco::Exception& ex)
            {
                printf("MYSQL Error: %s\n", ex.message().c_str());
                ASSERT(false);
            }
        }
    }

private:
    Statement _stmt;
    std::vector<PreparedStatementData> data;
};

class Database
{
public:
    Database();
    ~Database();
    
    bool Open(std::string connectionString);

    virtual void DoPreparedStatements() = 0;
    PreparedStatement* getPreparedStatement(Poco::UInt8 index);

protected:
    SessionPool* _pool;
    std::vector<PreparedStatement*> stmts;
};

#endif