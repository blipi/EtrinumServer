#include "Server.h"

//@ Net basic headers
// Including poco before is important, as it gives errors on Windows otherwise
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketAcceptor.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"

#include "AuthDatabase.h"
#include "debugging.h"
#include "CharactersDatabase.h"
#include "Cli.h"
#include "Client.h"
#include "defines.h"
#include "GridLoader.h"
#include "Log.h"
#include "ObjectManager.h"
#include "Object.h"
#include "Packet.h"
#include "Player.h"
#include "Tools.h"

using Poco::Net::SocketReactor;
using Poco::Net::SocketAcceptor;
using Poco::Net::ServerSocket;
using Poco::Timestamp;

// Crypting
#include <iostream>
#include <iomanip>

#include <files.h>
#include <modes.h>
#include <osrng.h>
#include <sha.h>
#include <aes.h>
#include <hmac.h>
#include <filters.h>

enum HANDLE_IF_TYPE
{
    TYPE_NULL = 0x00,

    TYPE_NO_HANDLE,
    TYPE_NOT_LOGGED_SKIP_HMAC,
    TYPE_NOT_LOGGED,
    TYPE_LOGGED,
    TYPE_IN_WORLD,
    TYPE_ALWAYS
};

enum OPCODES
{
    OPCODE_NULL                         = 0x00,
    
    // Server -> Client
    OPCODE_SC_EHLO                      = 0x9000,
    OPCODE_SC_TIME_OUT                  = 0x3001,
    OPCODE_SC_LOGIN_RESULT              = 0x5101,
    OPCODE_SC_SEND_CHARACTERS_LIST      = 0x5102,
    OPCODE_SC_SELECT_CHARACTER_RESULT   = 0x5103,
    OPCODE_SC_CREATE_CHARACTER_RESULT   = 0x5104,

    OPCODE_SC_SPAWN_OBJECT              = 0x5201,
    OPCODE_SC_DESPAWN_OBJECT            = 0x5202,
    OPCODE_SC_PLAYER_STATS              = 0x5203,
    
    // Client -> Server
    OPCODE_CS_EHLO                      = 0x9000,
    OPCODE_CS_KEEP_ALIVE                = 0x3000,
    OPCODE_CS_SEND_LOGIN                = 0x3101,
    OPCODE_CS_REQUEST_CHARACTERS        = 0x3102,
    OPCODE_CS_SELECT_CHARACTER          = 0x3103,
};

struct OpcodeHandleType
{
    OPCODES Opcode;
    struct _Handler
    {
        bool (Server::*handler)(Client*, Packet*);
        HANDLE_IF_TYPE HandleIf;
    }
    Handler;
};

const OpcodeHandleType Server::OpcodeTable[] = 
{
    // Client -> Server
    {OPCODE_CS_EHLO,                {&Server::handlePlayerEHLO,         TYPE_NOT_LOGGED_SKIP_HMAC   }},
    {OPCODE_CS_KEEP_ALIVE,          {NULL,                              TYPE_ALWAYS                 }},
    {OPCODE_CS_SEND_LOGIN,          {&Server::handlePlayerLogin,        TYPE_NOT_LOGGED             }},
    {OPCODE_CS_REQUEST_CHARACTERS,  {&Server::handleRequestCharacters,  TYPE_LOGGED                 }},
    {OPCODE_CS_SELECT_CHARACTER,    {&Server::handleCharacterSelect,    TYPE_LOGGED                 }},

    {OPCODE_NULL,                   {NULL,                              TYPE_NULL                   }},
};

#include <map>
typedef std::map<OPCODES, OpcodeHandleType::_Handler> OpcodeHash;
typedef std::pair<OPCODES, OpcodeHandleType::_Handler> OpcodeHashInserter;
OpcodeHash OpcodesMap;

#define WORLD_HEART_BEAT 50

/**
* Creates a new Server and binds to the port
*
* @param port The port where the servers binds
*/
Server::Server():
    _serverRunning(false)
{
    // Create the Opcodes Map
    for (int i = 0; ; ++i)
    {
        if (OpcodeTable[i].Opcode == OPCODE_NULL && OpcodeTable[i].Handler.HandleIf == TYPE_NULL && OpcodeTable[i].Handler.handler == NULL)
            break;

        OpcodesMap.insert(OpcodeHashInserter(OpcodeTable[i].Opcode, OpcodeTable[i].Handler));
    }

    // Reset all players online state
    PreparedStatement* stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_UPDATE_ONLINE_ONSTART);
    stmt->execute();
}

Server::~Server()
{
    
}

void Server::start(Poco::UInt16 port)
{
    // Create a server socket to listen.
    ServerSocket svs(port);
    
    // Create the reactor
    SocketReactor reactor(Poco::Timespan(15000000));
	
    // Create a SocketAcceptor
	SocketAcceptor<Client> acceptor(svs, reactor);

	// Run the reactor in its own thread so that we can wait for a termination request
	Thread reactorThread;
    reactorThread.start(reactor);

    // Flag that the server is running
    _serverRunning = true;

    // --------------- //
    // Server is ready //
    // --------------- //
    sLog.out(Message::PRIO_INFORMATION, "\n[OK] Server is running [Port: %d]\n", port);

    // Create a processor thread and wait for it
    Thread updater;
    updater.start(*this);

    CLI Cli;
    while (updater.isRunning())
    {
        if (!Cli.getCLI())
            _serverRunning = false;

        Thread::sleep(200);
    }

    // Close network acceptors
    reactor.stop();
    reactorThread.join();
}

#ifdef SERVER_FRAMEWORK_TEST_SUITE
    class Spawner
    {
    public:
        Spawner()
        {
            spawnLimit = count = 5000;
            playerMax = 1000;
            x = 0;
            z = 0;
        }

        ~Spawner()
        {}

        // Spawn 5 entities per tick
        // Avoid overload
        void spawn()
        {
            for (int i = 0; i < 50 && count > 0; i++, count--)
            {
                if ((i % 3 != 0) && playerMax > 0)
                {
                    SharedPtr<Player> plr = sObjectManager.createPlayer("ASD", NULL);
                    plr->Relocate(Vector2D(x, z));
                    sGridLoader.addObject(plr);
                    MotionMaster::StartSimpleMovement(plr, Vector2D(2800, 1000), SPEED_RUN);
                    playerMax--;
                }
                else
                {
                    SharedPtr<Object> obj = sObjectManager.create(HIGH_GUID_CREATURE);
                    obj->Relocate(Vector2D(x, z));
                    sGridLoader.addObject(obj);
                    MotionMaster::StartAngleMovement(obj, 0.5f, SPEED_WALK);
                }
                
                x += ((MAP_MAX_X - MAP_MIN_X) / (float)spawnLimit);
                z += ((MAP_MAX_Z - MAP_MIN_Z) / (float)spawnLimit);
            }
        }

    private:
        Poco::UInt32 spawnLimit;
        Poco::UInt32 count;
        Poco::UInt32 playerMax;
        float x;
        float z;
    };
#endif

void Server::run()
{
    #ifdef SERVER_FRAMEWORK_TEST_SUITE
        Spawner spawner;
    #endif

    Timestamp lastUpdate;
    Poco::UInt32 prevSleepTime = 0;
    while (_serverRunning)
    {
        // Microseconds to miliseconds, as we have no usleep function
        _diff = lastUpdate.elapsed() / 1000;
        lastUpdate.update();

        sLog.out(Message::PRIO_DEBUG, "Diff time: %d", _diff);
        //if (_diff > 125)
            //ASSERT(false);

        // Update all grids now
        sGridLoader.update(_diff);

        // Spawn mobs
        #ifdef SERVER_FRAMEWORK_TEST_SUITE
            spawner.spawn();
        #endif

        // Wait for a constant update time
        if (_diff <= WORLD_HEART_BEAT + prevSleepTime)
        {   
            prevSleepTime = WORLD_HEART_BEAT + prevSleepTime - (Poco::UInt32)_diff;
            Thread::sleep(prevSleepTime);
        }
        else
            prevSleepTime = 0;
    }
}

// Server -> Client packets
void Server::SendPlayerEHLO(Client* client)
{
    CryptoPP::AutoSeededRandomPool rng;
    Packet* packet = new Packet(OPCODE_SC_EHLO, 27); // 10 HMAC, 1 SEC, 16 AES Key

    for (Poco::UInt8 i = 0; i < 10; ++i)
        *packet << rng.GenerateByte();
   
    Poco::UInt32 sec = rng.GenerateByte();
    *packet << (Poco::UInt8)(sec & 0xFF);

    Poco::UInt8* key = client->GetAESKey();
    
    for (Poco::UInt8 i = 0; i < 16; ++i)
    {
        key[i] = rng.GenerateByte();
        *packet << key[i];
    }

    client->SetSecurityByte(sec);
    client->SetHMACKeyLow(packet->rawdata);

    client->sendPacket(packet, false, false);
}

void Server::SendClientDisconnected(Client* client)
{
    Packet* packet = new Packet(OPCODE_SC_TIME_OUT);
    client->sendPacket(packet);
}

Packet* Server::buildSpawnPacket(Object* object, bool deleteOnSend /*= true*/)
{
    Packet* packet = new Packet(OPCODE_SC_SPAWN_OBJECT, 2048, true, deleteOnSend);
    *packet << object->GetLowGUID();
    *packet << object->GetHighGUID();
    *packet << object->GetPosition().x;
    *packet << object->GetPosition().z;

    switch (object->GetHighGUID())
    {
        case HIGH_GUID_CREATURE:
        case HIGH_GUID_PLAYER:
        {
            Character* character = object->ToCharacter();

            *packet << character->GetName();
            *packet << character->GetSpeed(MOVEMENT_RUN);
            *packet << character->GetSpeed(MOVEMENT_WALK);
            *packet << character->MovementTypeSpeed();

            if (character->hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
            {
                *packet << Poco::UInt8(0x01);
                *packet << character->motionMaster.getMovementType();

                if (character->motionMaster.getMovementType() == MOVEMENT_TO_POINT)
                {
                    *packet << character->motionMaster.next().x;
                    *packet << character->motionMaster.next().z;
                }
                else if (character->motionMaster.getMovementType() == MOVEMENT_BY_ANGLE)
                    *packet << character->getFacingTo();
            }
            else
                *packet << Poco::UInt8(0x00);

            *packet << character->GetMaxHP();
            *packet << character->GetHP();
            *packet << character->GetMaxMP();
            *packet << character->GetMP();
            break;
        }
    }

    return packet;
}

void Server::sendPacketTo(Packet* packet, Object* to)
{
    if (Client* client = to->getClient())
        client->sendPacket(packet);
    else if (packet->DeleteOnSend)
        delete packet;
}

Packet* Server::buildDespawnPacket(Poco::UInt64 GUID)
{
    Packet* packet = new Packet(OPCODE_SC_DESPAWN_OBJECT, 8, false, false);
    *packet << LOGUID(GUID);
    *packet << HIGUID(GUID);

    return packet;
}

void Server::sendPlayerStats(Client* client, SharedPtr<Object> object)
{
    // We should send the STR, INT, attack, defense, and private attributes here
    //@todo: All the params, right now we are only sending the player guid
    Packet* packet = new Packet(OPCODE_SC_PLAYER_STATS, 8);
    *packet << object->GetLowGUID();
    *packet << object->GetHighGUID();

    client->sendPacket(packet, true);
}

bool Server::parsePacket(Client* client, Packet* packet, Poco::UInt8 securityByte)
{
    sLog.out(Message::PRIO_DEBUG, "[%d]\t[C->S] %.4X", client->GetId(), packet->opcode);

    OPCODES opcode = (OPCODES)packet->opcode;
    if (OpcodesMap.find(opcode) == OpcodesMap.end())
        return false;
    
    // Check packet sec byte (Avoid packet reordering/inserting)
    if (packet->sec != securityByte)
        return false;

    // There are special packets which don't include HMAC (Avoid packet modifying)
    if (OpcodesMap[opcode].HandleIf != TYPE_NOT_LOGGED_SKIP_HMAC)
        if (!checkPacketHMAC(client, packet))
            return false;

    if (packet->isEncrypted())
        decryptPacket(client, packet);

    switch (OpcodesMap[opcode].HandleIf)
    {
        case TYPE_NULL:
            // This shouldn't be here
            return false;

        case TYPE_NO_HANDLE:
            return true;
            
        case TYPE_NOT_LOGGED_SKIP_HMAC:
        case TYPE_NOT_LOGGED:
            if (client->getLogged() || client->getInWorld())
                return false;
            break;

        case TYPE_LOGGED:
            if (!client->getLogged() || client->getInWorld())
                return false;
            break;

        case TYPE_IN_WORLD:
            if (!client->getInWorld())
                return false;
            break;

        default:
            break;
    }

    if (!OpcodesMap[opcode].handler) // TODO: Report there's a missing function unless it's KEEP_ALIVE
        return true;

    return (this->*OpcodesMap[opcode].handler)(client, packet);
}

bool Server::checkPacketHMAC(Client* client, Packet* packet)
{
    if (CryptoPP::HMAC<CryptoPP::SHA1>* verifier = client->getHMACVerifier())
        return verifier->VerifyDigest(packet->digest, packet->rawdata, packet->getLength());
    return false;
}

void Server::decryptPacket(Client* client, Packet* packet)
{
    packet->len &= 0x5FFF;

    if (CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption* decryptor = client->getAESDecryptor())
        decryptor->ProcessData(packet->rawdata, packet->rawdata, packet->len);
}

bool Server::handlePlayerEHLO(Client* client, Packet* packet)
{
    client->SetHMACKeyHigh(packet->rawdata);
    client->SetupSecurity();
    return true;
}

bool Server::handlePlayerLogin(Client* client, Packet* packet)
{
    std::string username;
    std::string pass;

    *packet >> username;
    packet->readAsHex(pass, 16);

    //  0       1
    // u.id, u.online
    PreparedStatement* stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_LOGIN);
    stmt->bindString(0, username);
    stmt->bindString(1, pass);
    
    RecordSet rs = stmt->execute();
    
    Packet* resp = new Packet(OPCODE_SC_LOGIN_RESULT, 1);

    if (rs.moveFirst())
    {
        client->setLogged(true);

        if (rs[1].convert<Poco::UInt8>() == 1)
            *resp << Poco::UInt8(0x02); // Already logged in
        else
        {
            *resp << Poco::UInt8(0x01);
            client->SetId(rs[0].convert<Poco::UInt32>());

            // Set online status
            stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_UPDATE_ONLINE);
            stmt->bindUInt8(0, 0x01);
            stmt->bindUInt32(1, client->GetId());
            stmt->execute();
        }
    }
    else
        *resp << Poco::UInt8(0x00);

    // Write back
    client->sendPacket(resp);
    return true;
}

bool Server::handleRequestCharacters(Client* client, Packet* packet)
{
    Poco::UInt8 request;
    *packet >> request;

    switch (request)
    {
        case 0x01:
            return sendCharactersList(client);
            break;

        case 0x02:
            return sendCharacterCreateResult(client, packet);

        default:
            return false;
    }

    return false;
}

bool Server::sendCharactersList(Client* client)
{
    client->ClearCharacters();

    Packet* resp = new Packet(OPCODE_SC_SEND_CHARACTERS_LIST, 1024, true);

    //  1       2       3
    // c.id, c.model, c.name
    PreparedStatement* stmt = CharactersDatabase.getPreparedStatement(QUERY_CHARACTERS_SELECT_BASIC_INFORMATION);
    stmt->bindUInt32(0, client->GetId());
    
    RecordSet rs = stmt->execute();

    *resp << Poco::UInt8(rs.rowCount());
    if (rs.moveFirst())
    {
        do
        {
            Characters character = {rs[0].convert<Poco::UInt32>(), rs[1].convert<Poco::UInt32>(), rs[2].convert<std::string>()};
            client->AddCharacter(character);

            *resp << character.id;
            *resp << character.model;
            *resp << character.name;

            PreparedStatement* stmtItems = CharactersDatabase.getPreparedStatement(QUERY_CHARACTERS_SELECT_VISIBLE_EQUIPMENT);
            stmtItems->bindUInt32(0, client->GetId());

            RecordSet rsItems = stmtItems->execute();

            if (rsItems.moveFirst())
            {
                do
                {
                    *resp << Poco::UInt8(0x01);
                    *resp << rsItems[0].convert<Poco::UInt32>();
                }
                while (rsItems.moveNext());
            }
            *resp << Poco::UInt8(0x00);
        }
        while (rs.moveNext());
    }

    client->sendPacket(resp, true);

    return true;
}

bool Server::handleCharacterSelect(Client* client, Packet* packet)
{
    Poco::UInt32 characterID;
    *packet >> characterID;

    Packet* resp = new Packet(OPCODE_SC_SELECT_CHARACTER_RESULT, 1);
    Characters* character = client->FindCharacter(characterID);
    if (character)
        *resp << Poco::UInt8(0x01);
    else
        *resp << Poco::UInt8(0x00);

    client->sendPacket(resp);

    if (character)
    {
        OnEnterToWorld(client, characterID);
        return true;
    }

    return false;
}

bool Server::sendCharacterCreateResult(Client* client, Packet* packet)
{
    //Packet* resp = new Packet(OPCODE_SC_SEND_CHARACTERS_LIST, 1024, true);
    return true;
}

void Server::OnEnterToWorld(Client* client, Poco::UInt32 characterID)
{
    // Create the player (Object) and add it to the object list
    SharedPtr<Player> player = client->onEnterToWorld(characterID);
    if (!player.isNull())
    {
        client->setInWorld(true);

        // Send player information
        sendPlayerStats(client, player);
            
        // Add the player to the GridLoader system
        sGridLoader.addObject(player);

        // Send an spawn packet of itself
        sendPacketTo(buildSpawnPacket(player), player);
    }
    else
    {
        //@todo: Notify player that he can't connect
        //@todo: time out client
    }
}

