#ifndef BASIC_SERVER_ENTITIES_CLIENT_H
#define BASIC_SERVER_ENTITIES_CLIENT_H

#include <limits>
#include <list>

//@ Mutex and Locking
#include "Poco/RWLock.h"
#include "Poco/SharedPtr.h"
#include "Poco/AutoPtr.h"

//@ Basic Net connections
#include "Poco/Net/NetException.h"
#include "Poco/Net/SocketReactor.h"
#include "Poco/Net/SocketNotification.h"
#include "Poco/Net/StreamSocket.h"
#include "Poco/NObserver.h"
#include "Poco/FIFOBuffer.h"

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

using Poco::SharedPtr;
using Poco::AutoPtr;
using Poco::Net::SocketReactor;
using Poco::Net::ReadableNotification;
using Poco::Net::ShutdownNotification;
using Poco::Net::TimeoutNotification;
using Poco::Net::WritableNotification;
using Poco::Net::StreamSocket;
using Poco::NObserver;
using Poco::FIFOBuffer;

class Server;
class Player;
class Packet;

enum LogicFlags
{
    DISCONNECT_READY                    = 1,
    DISCONNECT_SEND_FLAGS               = 2,
    DISCONNECTED_TIME_OUT               = 4,
    DISCONNECTED_NETWORK_ERROR          = 8,
    DISCONNECTED_INCORRECT_DATA         = 16,
    DISCONNECTED_CONNECTION_CLOSED      = 32,
};

struct Characters
{
    Poco::UInt32 id;
    Poco::UInt32 model;
    std::string name;
};

class Client
{
public:
    Client(StreamSocket& socket, SocketReactor& reactor);
    ~Client();

    void onReadable(const AutoPtr<ReadableNotification>& pNf);
    void onShutdown(const AutoPtr<ShutdownNotification>& pNf);
    void onTimeout(const AutoPtr<TimeoutNotification>& pNf);
    void onWritable(const AutoPtr<WritableNotification>& pNf);
    void onWriteOut(bool& b);
    //void onWriteIn(bool& b);
    void cleanupBeforeDelete();

    SharedPtr<Player> onEnterToWorld(Poco::UInt32 characterID);
    void sendPacket(Packet* packet, bool encrypt = false, bool hmac = true);

    inline Poco::UInt32 GetId()
    {
        return _id;
    }

    inline void SetId(Poco::UInt32 id)
    {
        _id = id;
    }

    inline bool getLogged()
    {
        return _logged;
    }

    inline void setLogged(bool logged)
    {
        _logged = logged;
    }

    inline bool getInWorld()
    {
        return _inWorld;
    }

    inline void setInWorld(bool inWorld)
    {
        _inWorld = inWorld;
    }

    inline void SetFlag(Poco::UInt64 flag)
    {
        _logicFlags |= flag;
    }

    void ClearCharacters();
    void AddCharacter(Characters character);
    Characters* FindCharacter(Poco::UInt32 ID);

    inline Player* GetPlayer()
    {
        return _player;
    }

    void SetSecurityByte(Poco::UInt32 sec);
    void SetHMACKeyLow(Poco::UInt8* low);
    void SetHMACKeyHigh(Poco::UInt8* high);
    void SetupSecurity();

    inline Poco::UInt8 (&GetHMACKey())[20]
    {
        return _packetData.HMACKey;
    }

    inline Poco::UInt8 (&GetAESKey())[16]
    {
        return _packetData.AESKey;
    }

    inline CryptoPP::HMAC<CryptoPP::SHA1>* getHMACVerifier()
    {
        return _packetData.verifier;
    }

    inline CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption* getAESDecryptor()
    {
        return _packetData.AESDec;
    }

private:
    void generateSecurityByte();

private:
    std::list<Characters> _characters;
    SharedPtr<Player> _player;

    Poco::UInt32 _id;
    Poco::UInt32 _characterId;
    bool _logged;
    bool _inWorld;

    Poco::UInt64 _logicFlags;    
	std::list<Packet*> _writePackets;
    Poco::RWLock _writeLock;

    struct PacketData
    {
        Poco::UInt32 securityByte;
        Poco::UInt8 HMACKey[20];
        Poco::UInt8 AESKey[16];

        CryptoPP::HMAC<CryptoPP::SHA1>* verifier;
        CryptoPP::ECB_Mode<CryptoPP::AES>::Encryption* AESEnc;
        CryptoPP::ECB_Mode<CryptoPP::AES>::Decryption* AESDec;

        PacketData()
        {
            verifier = NULL;
            AESEnc = NULL;
            AESDec = NULL;
            securityByte = 0;
            memset(HMACKey, 0, 20);
            memset(AESKey, 0, 16);
        }
    };
    PacketData _packetData;
    
    // Client connection
	StreamSocket   _socket;
	SocketReactor& _reactor;

    // Packet reading
    Packet* _packet;
    Poco::UInt8 _packetStep;

    // Packet writing
    enum
    {
        BUFFER_SIZE = 1024
    };

    FIFOBuffer _writeBufferOut;
};

#endif