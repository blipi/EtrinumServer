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
    
    IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set (POCO_LIBRARIES_DIR "${POCO_LIBRARIES_DIR}/x86_64")
    else()
        set (POCO_LIBRARIES_DIR "${POCO_LIBRARIES_DIR}/i386")
    endif()
    
    set (LIB_DATA "PocoData")
    set (LIB_DATA_MYSQL "PocoDataMysql")
    set (LIB_FOUNDATION "PocoFoundation")
    set (LIB_NET "PocoNet")
    set (LIB_XML "PocoXML")
    
else ()
    if ( POCO_STATIC )
        set (POCO_LIBRARIES_EXT "md")
        if ( BUILD_MT )
            set (POCO_LIBRARIES_EXT "mt")
        endif ()
    else ()
        set (POCO_LIBRARIES_EXT "")
    endif ()
endif()
    
FIND_LIBRARY(LIB_DATA_RELEASE
  NAMES
    PocoData${POCO_LIBRARIES_EXT} ${LIB_DATA} 
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_DATA_DEBUG
  NAMES
    PocoData${POCO_LIBRARIES_EXT}d ${LIB_DATA} 
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_DATA_MYSQL_RELEASE
  NAMES
    PocoDataMySQL${POCO_LIBRARIES_EXT} ${LIB_DATA_MYSQL} 
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_DATA_MYSQL_DEBUG
  NAMES
    PocoDataMySQL${POCO_LIBRARIES_EXT}d ${LIB_DATA_MYSQL}  
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_FOUNDATION_RELEASE
  NAMES
    PocoFoundation${POCO_LIBRARIES_EXT} ${LIB_FOUNDATION}  
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_FOUNDATION_DEBUG
  NAMES
    PocoFoundation${POCO_LIBRARIES_EXT}d ${LIB_FOUNDATION}   
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_NET_RELEASE
  NAMES
    PocoNet${POCO_LIBRARIES_EXT} ${LIB_NET}   
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_NET_DEBUG
  NAMES
    PocoNet${POCO_LIBRARIES_EXT}d ${LIB_NET}   
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_XML_RELEASE
  NAMES
    PocoXML${POCO_LIBRARIES_EXT} ${LIB_XML}   
  PATHS
    ${POCO_LIBRARIES_DIR}
)

FIND_LIBRARY(LIB_XML_DEBUG
  NAMES
    PocoXML${POCO_LIBRARIES_EXT}d ${LIB_XML}   
  PATHS
    ${POCO_LIBRARIES_DIR}
)

IF ( UNIX )
    set( POCO_LIBRARIES
        ${LIB_DATA_RELEASE} 
        ${LIB_DATA_MYSQL_RELEASE} 
        ${LIB_FOUNDATION_RELEASE} 
        ${LIB_NET_RELEASE}
        ${LIB_XML_RELEASE}
      )
else ()
    if( CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE )
      set( POCO_LIBRARIES
        optimized ${LIB_DATA_RELEASE} optimized ${LIB_DATA_MYSQL_RELEASE} optimized ${LIB_FOUNDATION_RELEASE} optimized ${LIB_NET_RELEASE} optimized ${LIB_XML_RELEASE}
        debug ${LIB_DATA_DEBUG} debug ${LIB_DATA_MYSQL_DEBUG} debug ${LIB_FOUNDATION_DEBUG} debug ${LIB_NET_DEBUG} debug ${LIB_XML_DEBUG}
      )
    else()
      set( POCO_LIBRARIES
        ${LIB_DATA_RELEASE} 
        ${LIB_DATA_MYSQL_RELEASE} 
        ${LIB_FOUNDATION_RELEASE} 
        ${LIB_NET_RELEASE}
        ${LIB_XML_RELEASE}
      )
    endif()
endif()

MARK_AS_ADVANCED(LIB_DATA_RELEASE)
MARK_AS_ADVANCED(LIB_DATA_MYSQL_RELEASE)
MARK_AS_ADVANCED(LIB_FOUNDATION_RELEASE)
MARK_AS_ADVANCED(LIB_NET_RELEASE)    
MARK_AS_ADVANCED(LIB_XML_RELEASE)    

MARK_AS_ADVANCED(LIB_DATA_DEBUG)
MARK_AS_ADVANCED(LIB_DATA_MYSQL_DEBUG)
MARK_AS_ADVANCED(LIB_FOUNDATION_DEBUG)
MARK_AS_ADVANCED(LIB_NET_DEBUG)    
MARK_AS_ADVANCED(LIB_XML_DEBUG)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Poco DEFAULT_MSG
  POCO_LIBRARIES 
  POCO_ROOT_DIR
)

MARK_AS_ADVANCED(POCO_LIBRARIES)
