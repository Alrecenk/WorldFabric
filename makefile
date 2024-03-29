BUILD = bin
EMSCRIPTEN_BUILD = embin
SERVER_DIR = ./server/
WASM_OUT   = ./hosted/generated/
USR_DIR    =/usr
INCS_DIRS  =-I${USR_DIR}/include -Iinclude -I./include
LIBS_DIRS  =-L${USR_DIR}/lib
CPP_DEFS=
#CPP_DEFS   =-D=HAVE_CONFIG_H
CPP_OPTS   = -O3 -Wno-pessimizing-move
MAKEFLAGS += -j2
#-Wall
LIBS       = -lssl -lcrypto


SRC		   =${SERVER_DIR}source/SecureWebServer.cpp\
			${SERVER_DIR}source/TableServer.cpp\
			${SERVER_DIR}source/TimelineServer.cpp

SRC_INC	   = -I${SERVER_DIR}include

API_DIR    =./wasm/
API_INC	   = -I${API_DIR}include -I${API_DIR}source
EXPORTED_FUNCTIONS =\
	'_setPacketPointer',\
	'_getReturnSize',\
	'_setModel',\
	'_getUpdatedBuffers',\
	'_rayTrace',\
	'_scan',\
	'_nextAnimation',\
	'_malloc',\
	'_getTableNetworkRequest',\
	'_distributeTableNetworkData',\
	'_createPin',\
	'_deletePin',\
	'_setPinTarget',\
	'_getNodeTransform',\
	'_applyPins',\
	'_runTimelineUnitTests',\
	'_runTimeline',\
	'_getBallObjects',\
	'_getInitialTimelineRequest',\
	'_synchronizeTimeline',\
	'_setBallVelocity',\
	'_getTimelineRunStats',\
	'_popPendingQuickSends',\
	'_requestModel',\
	'_getFirstPersonPosition',\
	'_createVRMPins',\
	'_getBones',\
	'_getMeshInstances',\
	'_createMeshInstance',\
	'_setMeshInstance',\
	'_getNearestSolid',\
	'_setSolidPose',\
	'_setIKParams',\
	'_setAvatar',\
	'_smoothLine',\
	'_free'
EXTRA_EXPORTED_RUNTIME_METHODS=['ccall']
API_MAIN = ${API_DIR}source/api.cpp
API_SRC    =${API_DIR}source/Variant.cpp\
			${API_DIR}source/OptimizationProblem.cpp\
        	${API_DIR}source/GLTF.cpp\
			${API_DIR}source/TableInterface.cpp\
			${API_DIR}source/UnitTests.cpp\
			${API_DIR}source/Timeline.cpp\
			${API_DIR}source/TObject.cpp\
			${API_DIR}source/TEvent.cpp\
			${API_DIR}source/CollisionSystem.cpp\
			${API_DIR}source/KDNode.cpp\
			${API_DIR}source/KDLeaf.cpp\
			${API_DIR}source/KDBranch.cpp\
			${API_DIR}source/CreateObject.cpp\
			${API_DIR}source/BouncingBall.cpp\
			${API_DIR}source/BallWall.cpp\
			${API_DIR}source/MoveBouncingBall.cpp\
			${API_DIR}source/ChangeBallVelocity.cpp\
			${API_DIR}source/ApplyBallImpulse.cpp\
			${API_DIR}source/MeshInstance.cpp\
			${API_DIR}source/SetMeshInstance.cpp\
			${API_DIR}source/MeshLibrary.cpp\
			${API_DIR}source/ConvexShape.cpp\
			${API_DIR}source/ConvexSolid.cpp\
			${API_DIR}source/MoveSimpleSolid.cpp\
			${API_DIR}source/SetConvexSolid.cpp\
			${API_DIR}source/ApplySolidImpulse.cpp\
			${API_DIR}source/Polygon.cpp\
			${API_DIR}source/BSPNode.cpp\
			${API_DIR}source/RadialVolume.cpp\
			${API_DIR}source/Subcurve.cpp

			
default: all

all: serverexe wasm
serverexe: ${SERVER_DIR}source/Main.cpp
	g++ -pthread -std=c++17 ${CPP_OPTS} ${CPP_DEFS} -o Main.exe -I${API_DIR} ${API_INC} ${SRC_INC} ${INCS_DIRS} ${SERVER_DIR}source/Main.cpp ${API_SRC} ${SRC} ${LIBS_DIRS} ${LIBS} 
wasm: ${API_DIR}source/api.cpp
	emcc -std=c++17 -g -sEXPORT_NAME="'initializeCPPAPI'" -sMODULARIZE=1 -sALLOW_MEMORY_GROWTH=0 -sINITIAL_MEMORY=3584MB -sMAXIMUM_MEMORY=4GB -O3 -sASSERTIONS=1 ${API_MAIN} ${API_SRC} --post-js ${API_DIR}source/api_post.js -o ${WASM_OUT}api.js ${API_INC} -s"EXPORTED_FUNCTIONS=${EXPORTED_FUNCTIONS}" -s"EXPORTED_RUNTIME_METHODS=${EXTRA_EXPORTED_RUNTIME_METHODS}"
clean:
	$(RM) -r ${SERVER_OUT}Main.exe ${WASM_OUT}api.js ${WASM_OUT}api.wasm
