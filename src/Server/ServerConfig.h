#ifndef _SERVERFRAMEWORK_CONFIG_H
#define _SERVERFRAMEWORK_CONFIG_H

#include "Poco/Poco.h"
#include "Poco/SingletonHolder.h"
#include "Poco/AutoPtr.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/Node.h"

#include <map>
#include <string>

class Config
{
public:
    typedef std::map<std::string, std::string> StringConfigsMap;
    typedef std::map<std::string, int> IntConfigsMap;
    typedef std::map<std::string, bool> BoolConfigsMap;

    Config();
    static Config& instance()
    {
        static Poco::SingletonHolder<Config> sh;
        return *sh.get();
    }

    StringConfigsMap getDatabaseInformation();
    void readConfiguration();

    int getIntConfig(std::string configName);
    bool getBoolConfig(std::string configName);

    int getDefaultInt(std::string configName, int _default);
    bool getDefaultBool(std::string configName, int _default);

private:
    Poco::XML::Node* firstChild(Poco::XML::Node* node);
    Poco::XML::Node* nextNode(Poco::XML::Node* node);

private:
    Poco::AutoPtr<Poco::XML::Document> _doc;
    StringConfigsMap _connectionStrings;
    
    IntConfigsMap _intConfigs;
    BoolConfigsMap _boolConfigs;
};

#define sConfig Config::instance()

#endif