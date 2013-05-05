# Copyright (C) 2008-2013 EtrinumProject
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if ( MSVC )
    configure_file (src/config/msvc/sparseconfig.h ${CMAKE_CURRENT_BINARY_DIR}/src/sparsehash/internal/sparseconfig.h COPYONLY)
elseif ( CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX )
    configure_file (src/config/gcc/sparseconfig.h ${CMAKE_CURRENT_BINARY_DIR}/src/sparsehash/internal/sparseconfig.h COPYONLY)
else ()
    message(FATAL_ERROR "Could not configure sparsehash")
endif ()