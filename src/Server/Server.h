#ifndef BASIC_SERVER_SERVER_H
#define BASIC_SERVER_SERVER_H

#include <map>
#include <list>
#include <unordered_map>

//@ Basic Poco Types and Threading
#include "Poco/Poco.h"
#include "Poco/Thread.h"

//@ Shared Pointers to save objects
#include "Poco/SharedPtr.h"

#include "defines.h"

using Poco::SharedPtr;
using Poco::Thread;

class Object;
class Client;
class Packet;

struct OpcodeHandleType;

class Server
{
public:
    Server();
    ~Server();

    void start(Poco::UInt16 port);

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
    void UpdateVisibilityOf(Object* from, Object* to);
    void sendDespawnPacket(Poco::UInt64 GUID, Object* to);

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
    bool sendCharacterCreateResult(Client* client, Packet* packet);

    void sendPlayerStats(Client* client, SharedPtr<Object> object);
    void OnEnterToWorld(Client* client, Poco::UInt32 characterID);

private:
    bool _serverRunning;

    static const OpcodeHandleType OpcodeTable[];
};

#endif