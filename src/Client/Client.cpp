#include "Client.h"
#include "Server.h"
#include "Player.h"
#include "Packet.h"
#include "Grid.h"
#include "Tools.h"
#include "AuthDatabase.h"
#include "CharactersDatabase.h"

/**
* Client class constructor
*
* @param s Socket used by the client to connect
*/
Client::Client(const Poco::Net::StreamSocket& s):
    _player(NULL),
    _stop(false),
    _logicFlags(0),
    _logged(false),
    _inWorld(false),
    _id(0),
    Poco::Net::TCPServerConnection(s)
{
}

/**
* Client destructor
*/
Client::~Client()
{
    _player = NULL;
}

/**
* Called when a client connects and enters the world. The player (entity) is
* created at this step
*
* @param GUID User unique online ID
* @param characterID Which of the users in the character list is selected
* @return Player created entity
*/
Player* Client::onEnterToWorld(Poco::UInt64 GUID, Poco::UInt32 characterID)
{
    Characters* character = FindCharacter(characterID);
    if (!character)
        return NULL;

    _characterId = characterID;

    _player = new Player(character->name);
    _player->SetGUID(GUID);

    PreparedStatement* stmt = CharactersDatabase.getPreparedStatement(QUERY_CHARACTERS_SELECT_INFORMATION);
    stmt->bindUInt32(0, characterID);

    RecordSet rs = stmt->execute();
    if (rs.moveFirst())
    {
        //  0     1    2       3       4      5      6
        // c.x, c.y, c.maxhp, c.hp, c.maxmp, c.mp, c.lvl        

        _player->Relocate(Vector2D(Tools::u32tof(rs[0].convert<Poco::UInt32>()), Tools::u32tof(rs[1].convert<Poco::UInt32>())));
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
        delete _player;
        _player = NULL;
    }

    return _player;
}

/**
* TCP Socket thread, sends and receives packets
*/
void Client::run()
{
    // Once connection has been done
    // Create the client at the server
    sServer->newClient(this);
    // Send the handshake to the player
    sServer->SendPlayerEHLO(this);
    
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
    Poco::UInt8 packetStep = STEP_NEW_PACKET;
    Packet* packet = NULL;

    Poco::UInt8 lastRead = 0;
    Poco::Timespan timeOut(1, 0);
    while (!_stop)
    {
        _writeLock.readLock();
        if (!_writePackets.empty())
        {
            _writeLock.unlock();

            if (socket().poll(timeOut, Poco::Net::Socket::SELECT_WRITE))
            {
                _writeLock.writeLock();
                while (!_writePackets.empty())
                {
                    Packet* packet = _writePackets.front();
                    socket().sendBytes(&packet->len, sizeof(packet->len));
                    socket().sendBytes(&packet->opcode, sizeof(packet->opcode));
                    socket().sendBytes(&packet->sec, sizeof(packet->sec));
                    socket().sendBytes(packet->digest, sizeof(packet->digest));
                    socket().sendBytes(packet->rawdata, packet->getLength());
                    delete packet;

                    _writePackets.pop_front();
                }
                _writeLock.unlock();
            }
        }
        else
            _writeLock.unlock();

        if (socket().poll(timeOut, Poco::Net::Socket::SELECT_READ) == false)
        {
            // TODO: Config: Timeout interval
            lastRead++;
            if (lastRead == 15)
                _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_TIME_OUT;
        }
        else
        {
            int nBytes = -1;

            try
            {
                if (packetStep == STEP_NEW_PACKET)
                {
                    packet = new Packet();
                    packetStep = STEP_READ_LENGTH;
                }

                switch (packetStep)
                {
                    case STEP_READ_LENGTH:
                        nBytes = socket().receiveBytes(&packet->len, sizeof(packet->len));
                        break;
                    case STEP_READ_OPCODE:
                        nBytes = socket().receiveBytes(&packet->opcode, sizeof(packet->opcode));
                        break;
                    case STEP_READ_SEC:
                        nBytes = socket().receiveBytes(&packet->sec, sizeof(packet->sec));
                        break;
                    case STEP_READ_DIGEST:
                        nBytes = socket().receiveBytes(packet->digest, sizeof(packet->digest));
                        break;
                    case STEP_READ_DATA:
                        nBytes = socket().receiveBytes(packet->rawdata, packet->getLength());
                        break;
                }

                packetStep++;

                if (packetStep == STEP_READ_DATA)
                {
                    packet->rawdata = new Poco::UInt8[packet->getLength() + 1];

                    if (packet->getLength() == 0)
                        packetStep = STEP_END_PACKET;
                }

                if (packetStep == STEP_END_PACKET)
                {
                    lastRead = 0;
                    generateSecurityByte();
                    if (!sServer->parsePacket(this, packet, (Poco::UInt8)(_packetData.securityByte & 0xFF)))
                        _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_INCORRECT_DATA;

                    packetStep = STEP_NEW_PACKET;
                    delete packet;
                }
            }
            catch (Poco::Exception& /*ex*/)
            {
                //Handle your network errors.
                _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_NETWORK_ERROR;
            }

            // If bytes read are 0 and we are not already disconnecting, flag it
            if (nBytes == 0 && !(_logicFlags & DISCONNECT_ON_EMPTY_QUEUE))
                _logicFlags |= DISCONNECT_ON_EMPTY_QUEUE | DISCONNECTED_CONNECTION_CLOSED;
        }

        #if defined(SERVER_FRAMEWORK_TESTING)
            if (_logicFlags & DISCONNECT_ON_EMPTY_QUEUE)
                printf("Disconnect flags: %d\n", _logicFlags & ~DISCONNECT_ON_EMPTY_QUEUE);
        #endif
		
        if ((_logicFlags & DISCONNECTED_INCORRECT_DATA) || (_logicFlags & DISCONNECTED_TIME_OUT))
        {
            sServer->SendClientDisconnected(this);
            _logicFlags &= ~DISCONNECTED_INCORRECT_DATA;
            _logicFlags &= ~DISCONNECTED_TIME_OUT;
        }

        _writeLock.readLock();
        // Cleanups are done after all packet sending, so that everything can get updated before logging off ;)
	    if(_writePackets.empty() && (_logicFlags & DISCONNECT_ON_EMPTY_QUEUE))
	    {
            if (_inWorld)
            {
                // Stop movement if any
                _player->motionMaster.clear();
                _player->clearFlag(FLAGS_TYPE_MOVEMENT, FLAG_MOVING);

                // It ALWAYS must be deleted from Grid first, otherwise we may run into heap corruption
                if (_player->IsOnGrid())
                    sGridLoader.removeObject(_player->ToObject());

                // Send despawn packet to all nearby objects
                _player->Despawn();

                // Delete from the server object list
                sServer->removeObject(_player->GetGUID(), true);
            }

		    _stop = true;
	    }
	    _writeLock.unlock();
    }

    // Reset online status
    PreparedStatement* stmt = AuthDatabase.getPreparedStatement(QUERY_AUTH_UPDATE_ONLINE);
    stmt->bindInt8(0, 0);
    stmt->bindUInt32(1, GetId());
    stmt->execute();

    socket().close();
}

/**
* Adds a packet to the send list of the client
*
* @param packet The packet to be sent
*/
void Client::addWritePacket(Packet* packet)
{
	_writeLock.writeLock();
	_writePackets.push_back(packet);
	_writeLock.unlock();
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
