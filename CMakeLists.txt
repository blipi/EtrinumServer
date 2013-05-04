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

if( MSVC )
    option(OPENSSL_MT "Use OpenSSL MT libs instead of MD" ON)
    option(OPENSSL_STATIC "Use OpenSSL static libs instead of dynamic" ON)

    set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

find_package(OpenSSL REQUIRED)

if( UNIX )
    set (CMAKE_CXX_FLAGS "-std=c++0x")
    find_package(MySQL REQUIRED)
    find_package(ZLIB REQUIRED)
    
    option(CMAKE_OSX_ARCHITECTURES "OS Architecture" "x86_64")
elseif ( WIN32 )
    option(MYSQL_USE_SOURCES "Use mysqllite sources" ON)
    
    if ( NOT MYSQL_USE_SOURCES )
        find_package(MySQL REQUIRED)
    else()
        set(MYSQL_LIBRARIES libmysql)
    endif()
endif()

# add dependencies
add_subdirectory(dep)

# add core sources
add_subdirectory(src)
