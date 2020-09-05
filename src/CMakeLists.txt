
cmake_minimum_required (VERSION 2.8)
#set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#set (CMAKE_BUILD_TYPE DEBUG)

project (tex2web)

set (TopPath ${CMAKE_CURRENT_SOURCE_DIR}/..)
set (BinPath ${TopPath}/bin)
set (DepPath ${TopPath}/dep)
set (CxTopPath ${DepPath}/cx)

#### External Project ####
include(ExternalProject)

set (CxTopPath ${DepPath}/cx)

ExternalProject_Add(cx_project
  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/external/cx
  SOURCE_DIR ${CxTopPath}/src
  BUILD_ALWAYS 1
  INSTALL_COMMAND echo "No install step."
  )

#### The Rest ####

list (APPEND CFiles
  tex2web.c
  )

set (BldPath tex2web)
include (${CxTopPath}/src/include.cmake)

add_dependencies (cx cx_project)
add_dependencies (cembed cx_project)
add_dependencies (cswitch cx_project)
add_dependencies (comparispawn cx_project)

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
set (CPACK_RESOURCE_FILE_LICENSE "${TopPath}/LICENSE")
include (CPack)

enable_testing ()

set (TestPath ${TopPath}/test)

## Ensure that the examples we distribute actually works.
set (Examples
  hello
  table
  toc
  )

foreach (f ${Examples})
  add_test (NAME example_${f}
    COMMAND
    comparispawn ${TestPath}/expect/${f}.html
    ${BinPath}/tex2web -x ${TopPath}/example/${f}.tex -css style.css
    )
endforeach ()

add_test (NAME example_macro
  COMMAND
  comparispawn ${TestPath}/expect/macro.html
  ${BinPath}/tex2web -def pathname ../my/path -x ${TopPath}/example/macro.tex -css style.css
  )

add_test (NAME css
  COMMAND
  comparispawn ${TestPath}/expect/style.css
  ${BinPath}/tex2web -o-css -
  )

