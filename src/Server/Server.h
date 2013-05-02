#ifndef BASIC_SERVER_SERVER_H
#define BASIC_SERVER_SERVER_H

#include <map>
#include <list>
#include <unordered_map>

//@ Basic Poco Types and Threading
#include "Poco/Poco.h"
#include "Poco/Runnable.h"
#include "Poco/RWLock.h"

//@ Shared Pointers to save objects
#include "Poco/SharedPtr.h"

//@ Basic Net connections
#include "Poco/Net/TCPServer.h"

//@ Hash Map
#include "google/dense_hash_map"

#include "defines.h"

using Poco::SharedPtr;
using Poco::Net::Socket;
using Poco::Net::StreamSocket;
using Poco::Net::SocketAddress;


class Object;
class Client;
class Packet;

struct OpcodeHandleType;


class Server
{
public:
    //typedef std::unordered_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*object*/> ObjectMap;
    typedef google::dense_hash_map<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*object*/, std::hash<Poco::UInt64>, equint64> ObjectMap;
    typedef std::pair<Poco::UInt64 /*guid*/, Poco::SharedPtr<Object> /*object*/> ObjectMapInserter;
    typedef std::list<Client*> ClientList;

    Server(Poco::UInt16 port);
    ~Server();
    
    void newClient(Client* client);

    void newObject(Poco::SharedPtr<Object> object);
    SharedPtr<Object> GetObject(Poco::UInt64 GUID);
    void removeObject(Poco::UInt64 GUID, bool force = false);
    Client* getClient(Poco::UInt64 objectGUID);

    inline bool isRunning()
    {
        return _serverRunning;
    }
    inline void setRunning(bool running)
    {
        _serverRunning = running;
    }

    // Server -> Client packets
    void SendPlayerEHLO(Client* client);
    void SendClientDisconnected(Client* client);
    void UpdateVisibilityOf(Object* from, Object* to, bool visible);

    // Packet parsing function
    bool parsePacket(Client* client, Packet* packet, Poco::UInt8 securityByte);

private:
    void setPacketHMAC(Client* client, Packet* packet);
    bool checkPacketHMAC(Client* client, Packet* packet);
    void encryptPacket(Client* client, Packet* packet);
    void decryptPacket(Client* client, Packet* packet);

    bool handlePlayerEHLO(Client* client, Packet* packet);
    bool handlePlayerLogin(Client* client, Packet* packet);

    bool handleRequestCharacters(Client* client, Packet* packet);
    bool sendCharactersList(Client* client);
    bool handleCharacterSelect(Client* client, Packet* packet);

    void sendPlayerStats(Client* client, SharedPtr<Object> object);
    void OnEnterToWorld(Client* client, Poco::UInt32 characterID);

private:
    Poco::Net::TCPServer* server;
    bool _serverRunning;

    ObjectMap _objectsList;
    Poco::RWLock _objectMapLock;
	    
    ClientList _clients;

    
    static const OpcodeHandleType OpcodeTable[];
};

#endif