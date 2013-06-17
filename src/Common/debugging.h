#ifndef GAMESERVER_DEBUGGING_H
#define GAMESERVER_DEBUGGING_H

#include <iostream>
#include "Poco/NestedDiagnosticContext.h"

#define ASSERT(assertion) { if (!(assertion)) { Poco::NDC::current().dump(std::cerr); *((volatile int*)NULL) = 0; } }

#endif