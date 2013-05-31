#ifndef GAMESERVER_PREPARED_STATEMENT_DATA_H
#define GAMESERVER_PREPARED_STATEMENT_DATA_H

#include "Poco/Poco.h"
#include "Poco/Data/Data.h"

using namespace Poco::Data;

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

    static void bind(std::size_t pos, const PreparedStatementData& obj, AbstractBinder* binder, AbstractBinder::Direction dir)
    {
        poco_assert_dbg (binder != 0);

        switch (obj.dataType)
        {
            case DATA_STRING:   TypeHandler<std::string>::bind(pos++, obj.str, binder, dir);    break;
            case DATA_UINT8:    TypeHandler<Poco::UInt8>::bind(pos++, obj.u8, binder, dir);     break;
            case DATA_UINT16:   TypeHandler<Poco::UInt16>::bind(pos++, obj.u16, binder, dir);   break;
            case DATA_UINT32:   TypeHandler<Poco::UInt32>::bind(pos++, obj.u32, binder, dir);   break;
            case DATA_UINT64:   TypeHandler<Poco::UInt64>::bind(pos++, obj.u64, binder, dir);   break;
            case DATA_INT8:     TypeHandler<Poco::Int8>::bind(pos++, obj.i8, binder, dir);      break;
            case DATA_INT16:    TypeHandler<Poco::Int16>::bind(pos++, obj.i16, binder, dir);    break;
            case DATA_INT32:    TypeHandler<Poco::Int32>::bind(pos++, obj.i32, binder, dir);    break;
            case DATA_INT64:    TypeHandler<Poco::Int64>::bind(pos++, obj.i64, binder, dir);    break;
        }
    }

    static void prepare(std::size_t pos, const PreparedStatementData& obj, AbstractPreparator* prepare)
    {
        poco_assert_dbg (prepare != 0);

        switch (obj.dataType)
        {
            case DATA_STRING:   TypeHandler<std::string>::prepare(pos++, obj.str, prepare);    break;
            case DATA_UINT8:    TypeHandler<Poco::UInt8>::prepare(pos++, obj.u8, prepare);     break;
            case DATA_UINT16:   TypeHandler<Poco::UInt16>::prepare(pos++, obj.u16, prepare);   break;
            case DATA_UINT32:   TypeHandler<Poco::UInt32>::prepare(pos++, obj.u32, prepare);   break;
            case DATA_UINT64:   TypeHandler<Poco::UInt64>::prepare(pos++, obj.u64, prepare);   break;
            case DATA_INT8:     TypeHandler<Poco::Int8>::prepare(pos++, obj.i8, prepare);      break;
            case DATA_INT16:    TypeHandler<Poco::Int16>::prepare(pos++, obj.i16, prepare);    break;
            case DATA_INT32:    TypeHandler<Poco::Int32>::prepare(pos++, obj.i32, prepare);    break;
            case DATA_INT64:    TypeHandler<Poco::Int64>::prepare(pos++, obj.i64, prepare);    break;
        }
    }
};

}
}

#endif