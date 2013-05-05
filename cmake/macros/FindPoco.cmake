# - Try to find the OpenSSL encryption library
# Once done this will define
#
#  OPENSSL_ROOT_DIR - Set this variable to the root installation of OpenSSL
#
# Read-Only variables:
#  OPENSSL_FOUND - system has the OpenSSL library
#  OPENSSL_INCLUDE_DIR - the OpenSSL include directory
#  OPENSSL_LIBRARIES - The libraries needed to use OpenSSL

#=============================================================================
# Copyright 2006-2009 Kitware, Inc.
# Copyright 2006 Alexander Neundorf <neundorf@kde.org>
# Copyright 2009-2010 Mathieu Malaterre <mathieu.malaterre@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

# http://www.slproweb.com/products/Win32OpenSSL.html

set (POCO_ROOT_DIR "${CMAKE_SOURCE_DIR}/dep/poco")
    
IF ( NOT EXISTS "${POCO_ROOT_DIR}/")
    message ( FATAL_ERROR "Could not find Poco base dir")
endif ()

MARK_AS_ADVANCED(POCO_ROOT_DIR)

set (POCO_LIBRARIES_DIR "${POCO_ROOT_DIR}/lib")

IF ( NOT EXISTS "${POCO_LIBRARIES_DIR}/")
    message ( FATAL_ERROR "Could not find Poco libraries dir")
endif ()

IF ( UNIX )
    set (POCO_LIBRARIES_DIR "${POCO_LIBRARIES_DIR}/Linux")
    
    IF (PLATFORM EQUAL 64)
        set (POCO_LIBRARIES_DIR "${POCO_LIBRARIES_DIR}/x86_64")
    else()
        set (POCO_LIBRARIES_DIR "${POCO_LIBRARIES_DIR}/i386")
    endif()
    
    FIND_LIBRARY(POCO_LIBRARIES
      NAMES
        PocoData PocoDataMySQL PocoFoundation PocoNet
      PATHS
        ${POCO_LIBRARIES_DIR}
    )
else ()
    set (POCO_LIBRARIES_EXT "md")
    if ( BUILD_MT )
        set (POCO_LIBRARIES_EXT "mt")
    endif ()
    
    FIND_LIBRARY(LIB_POCO_RELEASE
      NAMES
        PocoData${POCO_LIBRARIES_EXT} PocoDataMySQL${POCO_LIBRARIES_EXT} PocoFoundation${POCO_LIBRARIES_EXT} PocoNet${POCO_LIBRARIES_EXT}
      PATHS
        ${POCO_LIBRARIES_DIR}
    )
    
    FIND_LIBRARY(LIB_POCO_DEBUG
      NAMES
        PocoData${POCO_LIBRARIES_EXT}d PocoDataMySQL${POCO_LIBRARIES_EXT}d PocoFoundation${POCO_LIBRARIES_EXT}d PocoNet${POCO_LIBRARIES_EXT}d
      PATHS
        ${POCO_LIBRARIES_DIR}
    )
    
    if( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
      set( POCO_LIBRARIES
        optimized ${LIB_POCO_RELEASE}
        debug ${LIB_POCO_DEBUG}
      )
    else()
      set( POCO_LIBRARIES
        ${SSL_EAY_RELEASE}
        ${LIB_EAY_RELEASE}
      )
    endif()

    MARK_AS_ADVANCED(LIB_POCO_DEBUG)
    MARK_AS_ADVANCED(LIB_POCO_RELEASE)    
    
endif()

MARK_AS_ADVANCED(POCO_LIBRARIES)
