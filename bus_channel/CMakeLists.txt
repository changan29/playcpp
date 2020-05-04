cmake_minimum_required (VERSION 3.2.0)

project (Router)
set(CMAKE_BUILD_TYPE DEBUG)
set(outdir .)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${outdir})

include_directories(.)
include_directories(.)
set(RouterSrc main.cpp Router.cpp  BusMgr.cpp )
add_executable(router ${RouterSrc})

set(logicSrc  Router.cpp logic.cpp  BusMgr.cpp )
add_executable(logic ${logicSrc}  )

set(ConnectSrc connect.cpp Router.cpp  BusMgr.cpp )
add_executable(connect  ${ConnectSrc}  )
