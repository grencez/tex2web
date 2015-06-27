
cmake_minimum_required (VERSION 2.8)
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#set (CMAKE_BUILD_TYPE DEBUG)

project (tex2web)

set (TopPath ${CMAKE_CURRENT_SOURCE_DIR}/..)
set (BinPath ${TopPath}/bin)
set (DepPath ${TopPath}/dep)
set (CxTopPath ${DepPath}/cx)
set (BldPath tex2web)

list (APPEND CFiles
  tex2web.c
  )

include (${CxTopPath}/src/include.cmake)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BinPath})

addbinexe (tex2web tex2web.c)

