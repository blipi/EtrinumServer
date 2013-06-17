#include "ServerConfig.h"
#include "debugging.h"

//@ XML reading
#include <fstream>
#include <string>
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NamedNodeMap.h"
#include "Poco/SAX/InputSource.h"
#include "Poco/Path.h"
#include "Poco/File.h"

ServerConfig::ServerConfig()
{
	Poco::Path path("Config.xml");
	Poco::File file(path);
	
	if (!file.exists())
	{
		// Log it out
		ASSERT(false);
	}
	
    std::ifstream in(path.toString());
    Poco::XML::InputSource src(in);
    Poco::XML::DOMParser parser;
    _doc = parser.parse(&src);
}

ServerConfig::StringConfigsMap ServerConfig::getDatabaseInformation()
{
    return _connectionStrings;
}

void ServerConfig::readConfiguration()
{
    Poco::XML::Node* baseNode = firstChild(firstChild(_doc));

    while(baseNode)
    {
        if (baseNode->nodeName().compare("database") == 0)
        {
            Poco::XML::Node* databaseNode = firstChild(baseNode);

            while (databaseNode)
            {
                std::string databaseName = databaseNode->nodeName();

                Poco::XML::Node* databaseKey = firstChild(databaseNode);

                while (databaseKey)
                {
                    if (!_connectionStrings[databaseName].empty())
                        _connectionStrings[databaseName].append(";");

                    _connectionStrings[databaseName].append(databaseKey->nodeName() + "=" + databaseKey->innerText());
                    databaseKey = nextNode(databaseKey);
                }

                databaseNode = nextNode(databaseNode);
            }
        }
        else if (baseNode->nodeName().compare("coreconfig") == 0)
        {
            Poco::XML::Node* configNode = firstChild(baseNode);

            while (configNode)
            {
                Poco::XML::NamedNodeMap* atribs = configNode->attributes();
                
                if (atribs->getNamedItem("type")->nodeValue().compare("int") == 0)
                    _intConfigs[configNode->nodeName()] = std::atoi(configNode->innerText().c_str());
                else if (atribs->getNamedItem("type")->nodeValue().compare("bool") == 0)
                    _boolConfigs[configNode->nodeName()] = std::atoi(configNode->innerText().c_str()) != 0;
				else if (atribs->getNamedItem("type")->nodeValue().compare("string") == 0)
					__stringConfigs[configNode->nodeName()] = configNode->innerText();

                configNode = nextNode(configNode);
            }
        }

        baseNode = nextNode(baseNode);
    }
}

int ServerConfig::getIntConfig(std::string configName)
{
    return getDefaultInt(configName, 0);
}

bool ServerConfig::getBoolConfig(std::string configName)
{
    return getDefaultBool(configName, false);
}

std::string ServerConfig::getStringConfig(std::string configName)
{
	return getDefaultString(configName, "");
}

int ServerConfig::getDefaultInt(std::string configName, int _default)
{
	IntConfigsMap::iterator itr = _intConfigs.find(configName);
    if (itr != _intConfigs.end())
        return _intConfigs[configName];
    return _default;
}
    
bool ServerConfig::getDefaultBool(std::string configName, bool _default)
{
	BoolConfigsMap::iterator itr = _boolConfigs.find(configName);
    if (itr != _boolConfigs.end())
        return *itr;
    return _default;
}

std::string ServerConfig::getDefaultString(std::string configName, bool _default)
{
	StringConfigsMap::iterator itr = _stringConfigs.find(configName);
	if (itr != _stringConfigs.end())
		return *itr;
	return _default;
}

Poco::XML::Node* ServerConfig::firstChild(Poco::XML::Node* node)
{
    if (!node)
        return NULL;

    Poco::XML::Node* key = node->firstChild();
    while (key && key->nodeType() != Poco::XML::Node::ELEMENT_NODE)
        key = key->nextSibling();
    
    return key;
}

Poco::XML::Node* ServerConfig::nextNode(Poco::XML::Node* node)
{
    if (!node)
        return NULL;

    Poco::XML::Node* key = node->nextSibling();
    while (key && key->nodeType() != Poco::XML::Node::ELEMENT_NODE)
        key = key->nextSibling();

    return key;
}
