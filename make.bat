::Windows batch file that emulates the behavior of calling "make" in a unix environment
@echo off

set USR_DIR=/usr
set INCS_DIRS=-I%USR_DIR%/include -Iinclude -I./include
set LIBS_DIRS=-L%USR_DIR%/lib
set CPP_DEFS=-D=HAVE_CONFIG_H
set CPP_OPTS=-Wall -O3 -Wno-pessimizing-move

set API_DIR=./wasm/
set API_INC=-I%API_DIR%include
set WASM_OUT=./hosted/generated/
set EXPORTED_FUNCTIONS=['_setPacketPointer',^
'_setModel',^
'_getUpdatedBuffers',^
'_rayTrace',^
'_paint',^
'_testAllocate',^
'_malloc',^
'_free']
set EXPORTED_RUNTIME_METHODS=['ccall']
set API_SRC=%API_DIR%source/Variant.cpp %API_DIR%source/TriangleMesh.cpp

@echo on
emcc -std=c++17 -g -s ALLOW_MEMORY_GROWTH=1 -O3 -s ASSERTIONS=1 -s EXPORT_NAME="initializeCPPAPI" -s MODULARIZE=1 -s MAXIMUM_MEMORY=4GB %API_DIR%source/api.cpp %API_SRC% --post-js %API_DIR%source/api_post.js -o %WASM_OUT%api.js %API_INC% -s "EXPORTED_FUNCTIONS=%EXPORTED_FUNCTIONS%" -s "EXPORTED_RUNTIME_METHODS=%EXPORTED_RUNTIME_METHODS%"