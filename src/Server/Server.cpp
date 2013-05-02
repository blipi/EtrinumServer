#include "Server.h"
#include "defines.h"
#include "Puller.h"
#include "Object.h"
#include "Player.h"
#include "Client.h"
#include "Packet.h"
#include "Grid.h"
#include "Tools.h"
#include "AuthDatabase.h"
#include "CharactersDatabase.h"
#include "debugging.h"

#include "Poco/Net/TCPServerParams.h"
#include "Poco/Net/TCPServerConnectionFactory.h"
#include "Poco/Net/Socket.h"

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

    OPCODE_SC_SPAWN_OBJECT              = 0x5201,
    OPCODE_SC_DESPAWN_OBJECT            = 0x5202,

    
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


/**
* Creates a new Server and binds to the port
*
* @param port The port where the servers binds
*/
Server::Server(Poco::UInt16 port):
    _serverRunning(true)
{
    // Create the Opcodes Map
    for (int i = 0; ; i++)
    {
        if (OpcodeTable[i].Opcode == OPCODE_NULL && OpcodeTable[i].Handler.HandleIf == TYPE_NULL && OpcodeTable[i].Handler.handler == NULL)
            break;

        OpcodesMap.insert(OpcodeHashInserter(OpcodeTable[i].Opcode, OpcodeTable[i].Handler));
    }

    // Init hash map
    _objectsList.set_empty_key(NULL);
    _objectsList.set_deleted_key(std::numeric_limits<Poco::UInt64>::max());

    // Reset all players online state
    PreparedStatement* stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_UPDATE_ONLINE_ONSTART);
    stmt->execute();

    //Create a server socket to listen.
    Poco::Net::ServerSocket svs(port);
    
    //Configure some server params.
    Poco::Net::TCPServerParams* pParams = new Poco::Net::TCPServerParams();
    pParams->setMaxThreads(1000);   // Max clients
    pParams->setMaxQueued(1000);    // Max login queue
    pParams->setThreadIdleTime(100);

    //Create your server
    server = new Poco::Net::TCPServer(new Poco::Net::TCPServerConnectionFactoryImpl<Client>(), svs, pParams);
    server->start();
}

Server::~Server()
{
    delete server;
}

/**
* Setups a new Client connection when stablished
* This is called by the client thread
*
* @param client Client object already created
*/
void Server::newClient(Client* client)
{
    // TODO: Thread safe!!!!!
    _clients.push_back(client);
}

/**
* Returns an Object given it's GUID
*
* @param GUID Object GUID including its High GUID
* @return NULL (SharedPtr<Object>(NULL) -> isNUll() = true) if not found, the Object otherwise
*/
SharedPtr<Object> Server::GetObject(Poco::UInt64 GUID)
{
    SharedPtr<Object> o = NULL;

    _objectMapLock.readLock();
    ObjectMap::iterator itr = _objectsList.find(GUID);
    if (itr != _objectsList.end())
        o = itr->second;
    _objectMapLock.unlock();

    return o;
}

/**
* Adds a new Object to the server
*
* @param object Assigned SharedPtr to the object to be added
*/
void Server::newObject(Poco::SharedPtr<Object> object)
{
    _objectMapLock.writeLock();
    _objectsList.insert(ObjectMapInserter(object->GetGUID(), object));
    _objectMapLock.unlock();
}

/**
* Remove an object from the server objects list
*
* @param GUID Object Ingame ID, including Type High GUID
*/
void Server::removeObject(Poco::UInt64 GUID, bool force /*= false*/)
{
    // The object MUST be there, if not found, crash!!
    _objectMapLock.readLock();
    ASSERT(_objectsList.find(GUID) != _objectsList.end());
    _objectMapLock.unlock();

    // If it's a player it requires special handling, the client itself will remove the
    // Object when complete unload is done by passing force = true
    bool isPlayer = ((GUID >> 32) & HIGH_GUID_PLAYER);
    if (force || !isPlayer)
    {
        _objectMapLock.writeLock();

        _objectsList.erase(GUID);
        if (isPlayer)
            _clients.remove(getClient(GUID));

        _objectMapLock.unlock();
    }

    if (isPlayer && !force)
    {
        for (ClientList::iterator itr = _clients.begin(); itr != _clients.end(); itr++)
        {
            if ((*itr)->GetPlayer()->GetGUID() == GUID)
            {
                (*itr)->SetFlag(DISCONNECT_ON_EMPTY_QUEUE);
                break;
            }
        }
    }
}

Client* Server::getClient(Poco::UInt64 objectGUID)
{
    for (ClientList::iterator itr = _clients.begin(); itr != _clients.end(); itr++)
        if ((*itr)->GetPlayer()->GetGUID() == objectGUID)
            return *itr;

    return NULL;
}

// Server -> Client packets
void Server::SendPlayerEHLO(Client* client)
{
    CryptoPP::AutoSeededRandomPool rng;
    Packet* packet = new Packet(OPCODE_SC_EHLO, 27); // 10 HMAC, 1 SEC, 16 AES Key

    for (Poco::UInt8 i = 0; i < 10; i++)
        *packet << rng.GenerateByte();
   
    Poco::UInt32 sec = rng.GenerateByte() ;
    *packet << (Poco::UInt8)(sec & 0xFF);

    Poco::UInt8* key = client->GetAESKey();
    
    for (Poco::UInt8 i = 0; i < 16; i++)
    {
        key[i] = rng.GenerateByte();
        *packet << key[i];
    }

    client->SetSecurityByte(sec);
    client->SetHMACKeyLow(packet->rawdata);

    client->addWritePacket(packet);
}

void Server::SendClientDisconnected(Client* client)
{
    Packet* packet = new Packet(OPCODE_SC_TIME_OUT);
    setPacketHMAC(client, packet);
    client->addWritePacket(packet);
}

void Server::UpdateVisibilityOf(Object* from, Object* to, bool visible)
{
    Packet* packet = NULL;

    if (!visible)
    {
        packet = new Packet(OPCODE_SC_DESPAWN_OBJECT, 4);
        *packet << from->GetLowGUID();
    }
    else
    {
        packet = new Packet(OPCODE_SC_SPAWN_OBJECT, 2048, true);
        *packet << from->GetLowGUID();
        *packet << from->GetHighGUID();
        *packet << Tools::getU32(from->GetPosition().x);
        *packet << Tools::getU32(from->GetPosition().x);
        
        switch (from->GetHighGUID())
        {
            case HIGH_GUID_CREATURE:
            case HIGH_GUID_PLAYER:
                {
                    Character* character = from->ToCharacter();

                    *packet << character->GetName();
                    *packet << Tools::getU32(character->GetSPeed(MOVEMENT_RUN));
                    *packet << Tools::getU32(character->GetSPeed(MOVEMENT_WALK));
                    *packet << character->MovementType();

                    if (character->hasFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING))
                    {
                        *packet << Poco::UInt8(0x01);
                        *packet << Tools::getU32(character->motionMaster.next().x);
                        *packet << Tools::getU32(character->motionMaster.next().y);
                    }
                    else
                        *packet << Poco::UInt8(0x00);
                }
                break;
        }
    }

    if (Client* client = getClient(to->GetGUID()))
    {
        encryptPacket(client, packet);
        setPacketHMAC(client, packet);
        client->addWritePacket(packet);
    }
}

bool Server::parsePacket(Client* client, Packet* packet, Poco::UInt8 securityByte)
{
    printf("[%d]\t[C->S] %.4X\n", client->GetId(), packet->opcode);

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

void Server::setPacketHMAC(Client* client, Packet* packet)
{
    if (CryptoPP::HMAC<CryptoPP::SHA1>* verifier = client->getHMACVerifier())
        verifier->CalculateDigest(packet->digest, packet->rawdata, packet->getLength());
}

bool Server::checkPacketHMAC(Client* client, Packet* packet)
{
    if (CryptoPP::HMAC<CryptoPP::SHA1>* verifier = client->getHMACVerifier())
        return verifier->VerifyDigest(packet->digest, packet->rawdata, packet->getLength());
    return false;
}

void Server::encryptPacket(Client* client, Packet* packet)
{
    if (CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption* decryptor = client->getAESEncryptor())
    {
        CryptoPP::StreamTransformationFilter enc(*client->getAESEncryptor());
        for(Poco::UInt16 i = 0; i < packet->len; i++)
            enc.Put(packet->rawdata[i]);
        enc.MessageEnd();

        packet->len = (Poco::UInt16)(enc.MaxRetrievable() | 0xA000);
    
        delete [] packet->rawdata;
        packet->rawdata = new Poco::UInt8[(Poco::UInt32)enc.MaxRetrievable()];
        enc.Get(packet->rawdata, (size_t)enc.MaxRetrievable());

        packet->len |= 0xA000;
    }
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
    setPacketHMAC(client, resp);
    client->addWritePacket(resp);
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
        }
        while (rs.moveNext());
    }

    encryptPacket(client, resp);
    setPacketHMAC(client, resp);
    client->addWritePacket(resp);

    return true;
}

bool Server::handleCharacterSelect(Client* client, Packet* packet)
{
    Poco::UInt32 characterID;
    *packet >> characterID;

    Packet* resp = new Packet(OPCODE_SC_SELECT_CHARACTER_RESULT, 1);
    Characters* character = client->FindCharacter(characterID);
    if (character)
    {
        *resp << Poco::UInt8(0x01);
        OnEnterToWorld(client, characterID);
    }
    else
        *resp << Poco::UInt8(0x00);

    setPacketHMAC(client, resp);
    client->addWritePacket(resp);

    return character ? true : false;
}

void Server::OnEnterToWorld(Client* client, Poco::UInt32 characterID)
{
    // Get a new GUID for the client
    Poco::UInt32 GUID = sGuidManager.getNewGUID(HIGH_GUID_PLAYER);
    // If GUID is MAX_GUID, it means the servers has run out of guids! O_O (Shouldn't happen, that's uint32(0xFFFFFFFF))
    if (GUID != GuidManager::MAX_GUID)
    {
        // Create the player (Object) and add it to the object list
        if (Player* player = client->onEnterToWorld(MAKE_GUID(HIGH_GUID_PLAYER, GUID), characterID))
        {
            SharedPtr<Object> obj(player->ToObject());
            newObject(obj);
            client->setInWorld(true);

            // Before adding the object to the GridLoader system, and therefore risking to run
            // Into threading issues, update visibility for nearby players (spawn)
            player->UpdateLoS();

            // Add the player to the GridLoader system
            sGridLoader.addObject(obj);
        }
    }
    else
    {
        throw new Poco::ApplicationException("Server has run out of GUIDs\n");
        setRunning(false);
    }
}

