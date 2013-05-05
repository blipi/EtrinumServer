#include "Config.h"

//@ XML reading
#include <fstream>
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/NodeIterator.h"
#include "Poco/DOM/NodeFilter.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/SAX/InputSource.h"

namespace Config
{
    DatabaseConnectionsMap readDatabaseInformation()
    {
        DatabaseConnectionsMap connectionStrings;

        std::ifstream in("Config.xml");
        Poco::XML::InputSource src(in);
        Poco::XML::DOMParser parser;
        Poco::AutoPtr<Poco::XML::Document> doc = parser.parse(&src);
        Poco::XML::Node* node = doc->firstChild();
        while (node)
        {
            if (node->nodeType() != Poco::XML::Node::ELEMENT_NODE)
            {
                node = node->nextSibling();
                continue;
            }

            if (node->nodeName().compare("database") == 0)
            {
                Poco::XML::Node* database = node->firstChild();

                while (database)
                {
                    if (database->nodeType() != Poco::XML::Node::ELEMENT_NODE)
                    {
                        database = database->nextSibling();
                        continue;
                    }

                    Poco::XML::Node* key = database->firstChild();

                    while (key)
                    {
                        if (key->nodeType() != Poco::XML::Node::ELEMENT_NODE)
                        {
                            key = key->nextSibling();
                            continue;
                        }

                        if (!connectionStrings[database->nodeName()].empty())
                            connectionStrings[database->nodeName()].append(";");
                        connectionStrings[database->nodeName()].append(key->nodeName() + "=" + key->innerText());

                        key = key->nextSibling();
                    }


                    database = database->nextSibling();
                }
            }

            node = node->nextSibling();
        }

        return connectionStrings;
    }
}
