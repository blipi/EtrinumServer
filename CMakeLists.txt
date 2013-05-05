# Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Set projectname (must be done AFTER setting configurationtypes)
project(EtrinumProjectServer)

# CMake policies (can not be handled elsewhere)
cmake_minimum_required(VERSION 2.8)
cmake_policy(SET CMP0005 OLD)

# add this options before PROJECT keyword
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

# set macro-directory
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

# build in Release-mode by default if not explicitly set
if( NOT CMAKE_BUILD_TYPE )
  set(CMAKE_BUILD_TYPE "Release")
endif()

option(CMAKE_INSTALL_PREFIX "Install location" "./Install")

if( WIN32 )
    option(BUILD_MT "Build and link MT libs instead of MD" OFF)
    if ( BUILD_MT )
        set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} /MT /O1 /Ob1 /D NDEBUG")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MT /Zi /O2 /Ob1 /D NDEBUG")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")    
    endif ()
    
    option(OPENSSL_STATIC "Use OpenSSL static libs instead of dynamic" ON)    
endif()

# Clear if MT/MD has been toggled
IF ( NOT BUILD_MT STREQUAL OLD_BUILD_MT )
    set(OLD_BUILD_MT ${BUILD_MT} CACHE INTERNAL "Previous value for BUILD_MT")
    
    set(LIB_EAY_DEBUG "LIB_EAY_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_EAY_RELEASE "LIB_EAY_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(SSL_EAY_DEBUG "SSL_EAY_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(SSL_EAY_RELEASE "SSL_EAY_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    
    
    set(LIB_DATA_RELEASE "LIB_DATA_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_DATA_MYSQL_RELEASE "LIB_DATA_MYSQL_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_FOUNDATION_RELEASE "LIB_FOUNDATION_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_NET_RELEASE "LIB_NET_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_XML_RELEASE "LIB_XML_RELEASE-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_DATA_DEBUG "LIB_DATA_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_DATA_MYSQL_DEBUG "LIB_DATA_MYSQL_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_FOUNDATION_DEBUG "LIB_FOUNDATION_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_NET_DEBUG "LIB_NET_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
    set(LIB_XML_DEBUG "LIB_XML_DEBUG-NOTFOUND" CACHE FILEPATH "Cleared." FORCE)
endif ()

find_package(OpenSSL REQUIRED)
find_package(Poco REQUIRED)

if( UNIX )
    set (CMAKE_CXX_FLAGS "-std=c++0x")
    find_package(MySQL REQUIRED)
    find_package(ZLIB REQUIRED)
    
elseif ( WIN32 )
    option(MYSQL_USE_SOURCES "Use mysqllite sources" ON)
    
    if ( NOT MYSQL_USE_SOURCES )
        find_package(MySQL REQUIRED)
    else()
        set(MYSQL_LIBRARIES libmysql)
    endif()
endif()

option (POCO_STATIC "Build Poco statically" ON)
if (POCO_STATIC)
    add_definitions(-DPOCO_STATIC)
endif ()

# add dependencies
add_subdirectory(dep)

# add core sources
add_subdirectory(src)

# add distribute directory
add_subdirectory(dist)
