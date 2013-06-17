#ifndef GAMESERVER_DATABASE_H
#define GAMESERVER_DATABASE_H

#include "Poco/Poco.h"
#include "Poco/Data/Column.h"
#include "Poco/Data/SessionPool.h"
#include "Poco/Data/MySQL/MySQL.h"
#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"

#include "debugging.h"
#include "PreparedStatement.h"

using namespace Poco::Data;

#define DO_PREPARED_STATEMENT(a, b) stmts[a] = new PreparedStatement( (_pool->get() << b) );

class Database
{
public:
    Database();
    ~Database();
    
    void Open(std::string connectionString);

    virtual void DoPreparedStatements() = 0;
    PreparedStatement* getPreparedStatement(Poco::UInt8 index);

protected:
    SessionPool* _pool;
    std::vector<PreparedStatement*> stmts;
};

#endif