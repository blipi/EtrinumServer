#include "Client.h"
#include "Server.h"
#include "Player.h"
#include "Packet.h"
#include "Grid.h"
#include "Tools.h"
#include "ObjectManager.h"
#include "AuthDatabase.h"
#include "CharactersDatabase.h"
#include "Log.h"


// Packet reading steps
enum PACKET_READING_STEPS
{
    STEP_NEW_PACKET = 0,
    STEP_READ_LENGTH,
    STEP_READ_OPCODE,
    STEP_READ_SEC,
    STEP_READ_DIGEST,
    STEP_READ_DATA,
    STEP_END_PACKET,
};

/**
* Client class constructor
*
* @param s Socket used by the client to connect
*/
Client::Client(StreamSocket& socket, SocketReactor& reactor):
	_socket(socket), _reactor(reactor),
    _packet(NULL), _packetStep(STEP_NEW_PACKET),
    _player(NULL),
    _logicFlags(0),
    _logged(false), _inWorld(false), _id(0),
	_writeBufferOut(BUFFER_SIZE, true)
{
    sLog.out(Message::PRIO_INFORMATION, "Connection from " + socket.peerAddress().toString());

    // Set reactor handlers
    _reactor.addEventHandler(_socket, NObserver<Client, ReadableNotification>(*this, &Client::onReadable));
	_reactor.addEventHandler(_socket, NObserver<Client, ShutdownNotification>(*this, &Client::onShutdown));
	_reactor.addEventHandler(_socket, NObserver<Client, TimeoutNotification>(*this, &Client::onTimeout));
    
    // Send the handshake to the player
    sServer->SendPlayerEHLO(this);
}

/**
* Client destructor
*/
Client::~Client()
{
    _player = NULL;
    if (_packetStep != Poco::UInt8(STEP_NEW_PACKET))
        delete _packet;

    _reactor.removeEventHandler(_socket, NObserver<Client, ReadableNotification>(*this, &Client::onReadable));
	_reactor.removeEventHandler(_socket, NObserver<Client, ShutdownNotification>(*this, &Client::onShutdown));
	_reactor.removeEventHandler(_socket, NObserver<Client, TimeoutNotification>(*this, &Client::onTimeout));

    _socket.shutdown();
    _socket.close();
}

/**
* Called when a client connects and enters the world. The player (entity) is
* created at this step
*
* @param GUID User unique online ID
* @param characterID Which of the users in the character list is selected
* @return Player created entity
*/
SharedPtr<Player> Client::onEnterToWorld(Poco::UInt32 characterID)
{
    Characters* character = FindCharacter(characterID);
    if (!character)
        return NULL;

    _characterId = characterID;

    _player = sObjectManager.createPlayer(character->name, this);
    if (_player.isNull())
        return NULL;

    PreparedStatement* stmt = CharactersDatabase.getPreparedStatement(QUERY_CHARACTERS_SELECT_INFORMATION);
    stmt->bindUInt32(0, characterID);

    RecordSet rs = stmt->execute();
    if (rs.moveFirst())
    {
        //  0     1    2       3       4      5      6
        // c.x, c.y, c.maxhp, c.hp, c.maxmp, c.mp, c.lvl        

        _player->Relocate(Vector2D(rs[0].convert<Poco::UInt32>(), rs[1].convert<Poco::UInt32>()));
        _player->SetMaxHP(rs[2].convert<Poco::UInt32>());
        _player->SetHP(rs[3].convert<Poco::UInt32>());
        _player->SetMaxMP(rs[4].convert<Poco::UInt32>());
        _player->SetMP(rs[5].convert<Poco::UInt32>());
        _player->SetLVL(rs[6].convert<Poco::UInt32>());
        
        stmt = CharactersDatabase.getPreparedStatement(QUERY_CHARACTERS_UPDATE_GUID);
        stmt->bindUInt32(0, _player->GetLowGUID());
        stmt->bindUInt32(1, characterID);
        stmt->execute();
    }
    else
    {
        sObjectManager.removeObject(_player->GetGUID());
        _player = NULL;
    }

    sLog.out(Message::PRIO_DEBUG, "Player created at (%.2f, %.2f)", _player->GetPosition().x, _player->GetPosition().z);

    return _player;
}

/**
* TCP Socket thread, sends and receives packets
*/
void Client::onReadable(const AutoPtr<ReadableNotification>& pNf)
{
    if (!(_logicFlags & DISCONNECT_ON_EMPTY_QUEUE))
    {
        if (_packetStep == STEP_NEW_PACKET)
        {
            _packet = new Packet();
            _packetStep = STEP_READ_LENGTH;
        }

        int nBytes = -1;
        try
        {
            switch (_packetStep)
            {
                case STEP_READ_LENGTH:
                    nBytes = _socket.receiveBytes(&_packet->len, sizeof(_packet->len));
                    break;
                case STEP_READ_OPCODE:
                    nBytes = _socket.receiveBytes(&_packet->opcode, sizeof(_packet->opcode));
                    break;
                case STEP_READ_SEC:
                    nBytes = _socket.receiveBytes(&_packet->sec, sizeof(_packet->sec));
                    break;
                case STEP_READ_DIGEST:
                    nBytes = _socket.receiveBytes(_packet->digest, sizeof(_packet->digest));
                    break;
                case STEP_READ_DATA:
                    nBytes = _socket.receiveBytes(_packet->rawdata, _packet->getLength());
                    break;
            }
        }
        catch (Poco::Net::ConnectionResetException ex)
        {
            printf("Client exception: %s\n", ex.what());
            _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_NETWORK_ERROR;
        }

        // If bytes read are 0 and we are not already disconnecting, flag it
        if (nBytes <= 0 && !(_logicFlags & DISCONNECT_ON_EMPTY_QUEUE))
            _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_CONNECTION_CLOSED;
        else
        {

            _packetStep++;
            if (_packetStep == STEP_READ_DATA)
            {
                _packet->rawdata = new Poco::UInt8[_packet->getLength() + 1];

                if (_packet->getLength() == 0)
                    _packetStep = STEP_END_PACKET;
            }

            if (_packetStep == STEP_END_PACKET)
            {
                generateSecurityByte();

                // @todo: Should we do this here? I believe we should do a queue and read them on a thread
                if (!sServer->parsePacket(this, _packet, (Poco::UInt8)(_packetData.securityByte & 0xFF)))
                    _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_INCORRECT_DATA;

                _packetStep = STEP_NEW_PACKET;
                delete _packet;
            }
        }
    }
    else
	{
        sLog.out(Message::PRIO_DEBUG, "Disconnect flags: %d\n", _logicFlags & ~DISCONNECT_ON_EMPTY_QUEUE);

        if (_logicFlags & DISCONNECTED_INCORRECT_DATA)
            sServer->SendClientDisconnected(this);

        cleanupBeforeDelete();
        _socket.close();
        
        delete this;
	}
}

void Client::onShutdown(const AutoPtr<ShutdownNotification>& pNf)
{
    cleanupBeforeDelete();
    delete this;
}

void Client::onTimeout(const AutoPtr<TimeoutNotification>& pNf)
{
    sServer->SendClientDisconnected(this);
    sLog.out(Message::PRIO_DEBUG, "Disconnect flags: %d\n", DISCONNECTED_TIME_OUT);

    cleanupBeforeDelete();
    delete this;
}

void Client::onWritable(const AutoPtr<WritableNotification>& pNf)
{
    _socket.sendBytes(_writeBufferOut);
}

void Client::cleanupBeforeDelete()
{
    if (_inWorld)
    {
        // Stop movement if any
        _player->motionMaster.clear();
        _player->clearFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);

        // Save GUID for further usage
        Poco::UInt64 GUID = _player->GetGUID();

        // If we are on a Grid (it is spawned), remove us
        if (_player->IsOnGrid())
            _player->GetGrid()->removeObject(_player->GetGUID());

        // Send despawn packet to all nearby objects
        _player->Despawn();

        // Delete from the server object list
        sObjectManager.removeObject(_player->GetGUID());

        // Flag it as not in world and not logged
        setInWorld(false);
        setLogged(false);
    }
        
    // Reset online status
    PreparedStatement* stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_UPDATE_ONLINE);
    stmt->bindInt8(0, 0);
    stmt->bindUInt32(1, GetId());
    stmt->execute();
}

/**
* Adds a packet to the send list of the client
*
* @param packet The packet to be sent
*/
void Client::sendPacket(Packet* packet, bool encrypt, bool hmac)
{
	sLog.out(Message::PRIO_DEBUG, "[%d]\t[S->C] %.4X", GetId(), packet->opcode);

    if (encrypt && _packetData.AESEnc)
    {
        CryptoPP::StreamTransformationFilter enc(*_packetData.AESEnc);
        for(Poco::UInt16 i = 0; i < packet->len; i++)
            enc.Put(packet->rawdata[i]);
        enc.MessageEnd();

        packet->len = (Poco::UInt16)(enc.MaxRetrievable() | 0xA000);
    
        delete [] packet->rawdata;
        packet->rawdata = new Poco::UInt8[(Poco::UInt32)enc.MaxRetrievable()];
        enc.Get(packet->rawdata, (size_t)enc.MaxRetrievable());

        packet->len |= 0xA000;
    }
    
    if (hmac && _packetData.verifier)
        _packetData.verifier->CalculateDigest(packet->digest, packet->rawdata, packet->getLength());

    _writeBufferOut.write((const char*)&packet->len, sizeof(packet->len));
    _writeBufferOut.write((const char*)&packet->opcode, sizeof(packet->opcode));
    _writeBufferOut.write((const char*)&packet->sec, sizeof(packet->sec));
    _writeBufferOut.write((const char*)packet->digest, sizeof(packet->digest));
    _writeBufferOut.write((const char*)packet->rawdata, packet->getLength());
    _writeBufferOut.drain(packet->getLength() + sizeof(packet->len) + sizeof(packet->opcode) + sizeof(packet->sec) + sizeof(packet->digest));
    
    /*_socket.sendBytes(&packet->len, sizeof(packet->len));
    _socket.sendBytes(&packet->opcode, sizeof(packet->opcode));
    _socket.sendBytes(&packet->sec, sizeof(packet->sec));
    _socket.sendBytes(packet->digest, sizeof(packet->digest));
    _socket.sendBytes(packet->rawdata, packet->getLength());
    */

    delete packet;
}

/**
* Generates a security byte for the client, it's updated at each
* packet, and it's used to check if there's been any injected packet
*/
void Client::generateSecurityByte()
{
    Poco::UInt32 result = (0x3F * (~_packetData.securityByte + 0x34));
	result = result ^ (result >> 4);
	result = result & 0xFF;
	_packetData.securityByte = result;
}

/**
* Resets the characters list
*/
void Client::ClearCharacters()
{
    _characters.clear();
}

/**
* Uppon load from DB, stores the client character
*
* @param character Character to be stored to the list
*/
void Client::AddCharacter(Characters character)
{
    _characters.push_back(character);
}

/**
* Static method used to find the searched packet
*
* @param character std::find passed argument
* @param ID character to be found ID
* @return true if it's the searched character
*/
static bool findCharacterByID(Characters character, Poco::UInt32 ID)
{
    return character.id == ID;
}

/**
* Finds a character given an ID
*
* @param ID character ID to be found
* @return The character if found
*/
Characters* Client::FindCharacter(Poco::UInt32 ID)
{
    std::list<Characters>::iterator itr = std::find_if(_characters.begin(), _characters.end(), std::bind2nd(std::ptr_fun(findCharacterByID), ID));
    if (itr == _characters.end())
        return NULL;

    return &(*itr);
}

/**
* Sets the security byte when a connection and handshake packet is stablished
*
* @param sec Security DWORD, only a BYTE is stored
*/
void Client::SetSecurityByte(Poco::UInt32 sec)
{
    _packetData.securityByte = sec;
}

/**
* Sets the low part of the HMAC key
*
* @param low 10 bytes array containing the low part
*/
void Client::SetHMACKeyLow(Poco::UInt8* low)
{
    for (Poco::UInt8 i = 0; i < 10; i++)
        _packetData.HMACKey[i] = low[i];
}

/**
* Sets the high part of the HMAC key
*
* @param low 10 bytes array containing the high part
*/
void Client::SetHMACKeyHigh(Poco::UInt8* high)
{
    for (Poco::UInt8 i = 0; i < 10; i++)
        _packetData.HMACKey[i+10] = high[i];
}

/**
* Sets the security handlers once the whole authentification has been done
*/
void Client::SetupSecurity()
{
    _packetData.verifier = new CryptoPP::HMAC<CryptoPP::SHA1>(_packetData.HMACKey, PACKET_HMAC_SIZE); 
    _packetData.AESEnc = new CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption(_packetData.AESKey, 16);
    _packetData.AESDec = new CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption(_packetData.AESKey, 16);
}
