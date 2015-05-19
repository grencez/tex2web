
cmake_minimum_required (VERSION 2.8)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#set (CMAKE_BUILD_TYPE DEBUG)

project (tex2web)

set (CxPath ${CMAKE_CURRENT_SOURCE_DIR}/../dep/cx)
set (BinPath ${CMAKE_CURRENT_SOURCE_DIR}/../bin)
set (BldPath tex2web)

list (APPEND CFiles
  tex2web.c
  )

include (${CxPath}/include.cmake)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BinPath})

addbinexe (tex2web tex2web.c)

