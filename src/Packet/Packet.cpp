#include "Packet.h"
#include <sstream>
#include <iostream>
#include <iomanip>
#include <string.h>

Packet::Packet()
{
	clear();
}

Packet::Packet(Poco::UInt16 _opcode, Poco::UInt16 size /*= 0*/, bool unknownLen /*= false*/)
{
	clear();
	if (size > 0)
		resize(size);
	opcode = _opcode;
	_unknownLen = unknownLen;

	if (!_unknownLen)
		len = size;
}

Packet::~Packet()
{
	if (rawdata)
		delete [] rawdata;
}

void Packet::clear()
{
	count = 0;
	len = 0;
	opcode = 0;
	sec = 0;
	memset(digest, 0, sizeof(digest));
	rawdata = NULL;
}
		
void Packet::operator << (std::string str)
{
	append<Poco::UInt16>((Poco::UInt16)str.length());
	for (std::string::iterator it = str.begin(); it != str.end(); it++)
		append<Poco::UInt8>((Poco::UInt8)*it);
}

void Packet::operator >> (std::string& value)
{
	Poco::UInt16 _len; 
	*this >> _len;
	value.resize(_len);

	for (int i = 0; i < _len; i++)
	{
		char c;
		*this >> c;
		value[i] = c;
	}
}

void Packet::readAsHex(std::string& value, Poco::UInt8 len)
{
    std::stringstream ss;

    for (Poco::UInt8 i = 0; i < len; i++)
    {
        Poco::UInt8 v;
        *this >> v;
        ss << std::setw(2) << std::setfill('0') << std::hex << int(v);
    }
    value = ss.str();
}
