
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
install (TARGETS tex2web DESTINATION bin)

# Build a CPack-driven installer package.
#cpack --config CPackConfig.cmake
include (InstallRequiredSystemLibraries)
string (TIMESTAMP CPACK_PACKAGE_VERSION "%Y.%m.%d")
#set (CPACK_PACKAGE_VERSION "2015.07.06")
set (CPACK_GENERATOR "STGZ;TGZ;ZIP")
set (MyName "Alex Klinkhamer (grencez)")
set (CPACK_PACKAGE_VENDOR ${MyName})
set (CPACK_PACKAGE_CONTACT ${MyName})
set (CPACK_RESOURCE_FILE_LICENSE "${TopPath}/COPYING")
include (CPack)

enable_testing ()

set (TestPath ${TopPath}/test)

## Ensure that the example we distribute actually works.
add_test (NAME example_hello
  COMMAND
  comparispawn ${TestPath}/expect/hello.html
  ${BinPath}/tex2web -x ${TopPath}/example/hello.tex
  )

add_test (NAME example_macro
  COMMAND
  comparispawn ${TestPath}/expect/macro.html
  ${BinPath}/tex2web -def pathname ../my/path -x ${TopPath}/example/macro.tex
  )

