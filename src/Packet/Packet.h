#ifndef BASIC_SERVER_PACKET_H
#define BASIC_SERVER_PACKET_H

#include "Poco/Poco.h"

#define PACKET_HMAC_SIZE        20

class Packet{
private:
	Poco::UInt32 count;
	bool _unknownLen;

	template <typename T>
	inline void append(T value){
		*(T*)((Poco::UIntPtr)rawdata + count) = value;

		count += sizeof(T);
		if(_unknownLen)
			len += sizeof(T);
	}

public:
	Poco::UInt16 len;
	Poco::UInt16 opcode;
	Poco::UInt8 sec;
	Poco::UInt8 digest[20];
	Poco::UInt8* rawdata;

	Packet();
	Packet(Poco::UInt16 _opcode, Poco::UInt16 size = 0, bool unknownLen = false);
    ~Packet();

    void clear();
		
	void operator << (std::string str);

	template <typename T>
	void operator << (T val)
    {
        append<T>(val);
    }
	
	void operator >> (std::string& value);
    void readAsHex(std::string& value, Poco::UInt8 len);

	template <typename T>
	void operator >> (T& value)
    {
        value = *(T*)(rawdata + count);
		count += sizeof(T);
    }

	inline void resize(size_t s)
	{
		if (rawdata)
			delete [] rawdata;
		rawdata = new Poco::UInt8 [s+1];
	}

    inline bool isEncrypted()
    {
        return (len & 0xA000) == 0xA000;
    }

	inline Poco::UInt16 getLength()
	{
        if (isEncrypted())
            return len & 0x5FFF;
		return len;
	}
};

#endif