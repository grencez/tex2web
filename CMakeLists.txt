
cmake_minimum_required (VERSION 2.8)

project (tex2web)

set (CxPath ../cx)
set (BinPath ../bin)
set (BldPath tex2web)

list (APPEND CFiles
  tex2web.c
  )

include (${CxPath}/include.cmake)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BinPath})

addbinexe (tex2web tex2web.c)

