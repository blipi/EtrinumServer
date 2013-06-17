#ifndef GAMESERVER_SERVERCONFIG_H
#define GAMESERVER_SERVERCONFIG_H

#include "Poco/Poco.h"
#include "Poco/SingletonHolder.h"
#include "Poco/AutoPtr.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Node.h"

#include <map>
#include <string>

class ServerConfig
{
public:
    typedef std::map<std::string, int> IntConfigsMap;
    typedef std::map<std::string, bool> BoolConfigsMap;
    typedef std::map<std::string, std::string> StringConfigsMap;

    ServerConfig();
    static ServerConfig& instance()
    {
        static Poco::SingletonHolder<ServerConfig> sh;
        return *sh.get();
    }

    StringConfigsMap getDatabaseInformation();
    void readConfiguration();

    int getIntConfig(std::string configName);
    bool getBoolConfig(std::string configName);
	std::string getStringConfig(std::string configName);

    int getDefaultInt(std::string configName, int _default);
    bool getDefaultBool(std::string configName, bool _default);
	std::string getDefaultString(std::string configName, std::string _default);
	
private:
    Poco::XML::Node* firstChild(Poco::XML::Node* node);
    Poco::XML::Node* nextNode(Poco::XML::Node* node);

private:
    Poco::AutoPtr<Poco::XML::Document> _doc;
    StringConfigsMap _connectionStrings;
    
    IntConfigsMap _intConfigs;
    BoolConfigsMap _boolConfigs;
	StringConfigsMap _stringConfigs;
};

#define sConfig ServerConfig::instance()

#endif