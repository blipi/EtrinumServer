#ifndef BASIC_SERVER_DEBUGGING_H
#define BASIC_SERVER_DEBUGGING_H

#include <iostream>
#include "Poco/NestedDiagnosticContext.h"

#define ASSERT(assertion) { if (!(assertion)) { Poco::NDC::current().dump(std::cerr); *((volatile int*)NULL) = 0; } }

#endif