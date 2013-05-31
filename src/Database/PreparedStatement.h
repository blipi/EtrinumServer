#ifndef GAMESERVER_PREPARED_STATEMENT_H
#define GAMESERVER_PREPARED_STATEMENT_H

#include "Poco/Poco.h"
#include "Poco/Data/Data.h"
#include "Poco/Data/RecordSet.h"
#include "Poco/Data/Statement.h"

using namespace Poco::Data;

class PreparedStatementData;

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
            }
        }
    }

private:
    Statement _stmt;
    std::vector<PreparedStatementData> data;
};

#endif