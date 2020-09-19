# Install script for directory: /home/orphey/lia_bot/ghostpp/bncsutil

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so.1.4.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/orphey/lia_bot/ghostpp/bncsutil/build/libbncsutil.so.1.4.1"
    "/home/orphey/lia_bot/ghostpp/bncsutil/build/libbncsutil.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so.1.4.1"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so"
         RPATH "")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/orphey/lia_bot/ghostpp/bncsutil/build/libbncsutil.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libbncsutil.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/bncsutil" TYPE FILE FILES
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/bncsutil.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/bsha1.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/buffer.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/cdkeydecoder.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/checkrevision.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/decodekey.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/file.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/keytables.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/libinfo.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/ms_stdint.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/mutil.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/mutil_types.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/nls.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/oldauth.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/pe.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/sha1.h"
    "/home/orphey/lia_bot/ghostpp/bncsutil/src/bncsutil/stack.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/home/orphey/lia_bot/ghostpp/bncsutil/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
