cmake_minimum_required (VERSION 3.6)

set(PROJECT_NAME UGS)

project (${PROJECT_NAME})

message(STATUS "bigin build project 'UGS' ")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(UGS_BUILD_SAMPLES "Build UGS Samples." ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

set(UGS_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin CACHE STRING "UGS Binary Dir" FORCE) #force update even if it in cache
message(STATUS "UGS_BINARY_DIR='${UGS_BINARY_DIR}' ")

set(_UGS_LOCATION_ ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "UGS Location" FORCE)

set(_UGS_SHADER_LOCATION_ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ CACHE STRING "UGS Shader Location" FORCE)


add_compile_definitions(UGS_LOCATION="${_UGS_LOCATION_}")
add_compile_definitions(UGS_SHADER_LOCATION="${_UGS_SHADER_LOCATION_}")

#######################################################################################
# Custom Lib & Link directory
#######################################################################################
link_directories( )


#######################################################################################
## Check compiler flag
#######################################################################################
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++17" _cpp_17_flag_supported)
    if (_cpp_17_flag_supported)
		message(STATUS "C++ 17 is available in current VS.")
        add_compile_options("/std:c++17")
    endif()
endif()

set(UGS_DEP_SHADER_FILES
	# Add explicit dependencies here
)

file(GLOB EFFECT11_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Optional/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Optional/*.cpp
)

file(GLOB DXUT_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Core/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Core/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Optional/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Optional/*.cpp

)

file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Core/
  ${CMAKE_CURRENT_SOURCE_DIR}/DXUT/Optional/
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/core
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/thirdparty/Assimp/include
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/thirdparty/
  ${CMAKE_CURRENT_SOURCE_DIR}/framework/thirdparty/imgui/
)


if (VERBOSE_MODE)
	## list include directory for debug
	get_property(includes DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
	foreach(dir ${includes})
		message(STATUS "${PROJECT_NAME} include dir='${dir}'")
	endforeach()
endif (VERBOSE_MODE)


#######################################################################################
## function definition
#######################################################################################
function(group_files src_files)
	foreach(f ${src_files})
        # replace "a/b/c" --> "a\\b\\c"
		string(REPLACE / \\\\ f ${f})
		# Get the path of the file relative to ${CMAKE_SOURCE_DIR}
		file(RELATIVE_PATH relative_path ${CMAKE_SOURCE_DIR} ${f})
		# Extract the folder, ie remove the filename part
		string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" relative_path ${relative_path})

		set(SRCGR "${relative_path}")
		# Source_group expects \\ (double antislash), not / (slash)
		string(REPLACE / \\\\ SRCGR ${SRCGR})
		#message(STATUS "1. group name=${SRCGR}, file=${f}")

        # regular expression https://cmake.org/cmake/help/latest/command/string.html#regex-specification
	    if ("${f}" MATCHES ".*\\.cpp$")
		   set(SRCGR "${SRCGR}\\\\cpp")
        elseif("${f}" MATCHES ".*\\.h$")
		   set(SRCGR "${SRCGR}\\\\header")
        endif()	

		if (VERBOSE_MODE)
				message(STATUS "2. group name=${SRCGR}, file=${f}")
		endif (VERBOSE_MODE)

		source_group(${SRCGR} FILES ${f})
	endforeach()
endfunction()

group_files("${UGS_EXT_FILES}")
group_files("${UGS_FILES}")
group_files("${UGS_INCLUDES}")
group_files("${UGS_SHADER_FILES}")

#######################################################################################
# Build Shader
#######################################################################################
find_program(FXC NAMES fxc.exe PATHS "C:\\Program Files (x86)\\Windows Kits\\10\\bin\\10.0.17763.0\\x64" )

function(shader_compile shaderfile dependencies)
	file(RELATIVE_PATH name ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${shaderfile})
	set(tempfile ${UGS_BINARY_DIR}/temp/${name}.temp)
	#message(STATUS "shaderfile='${shaderfile}' tempfile='${tempfile}' ")
	add_custom_command(
		OUTPUT ${tempfile}
		COMMAND ${FXC} /T vs_4_1 /E VS /Fo ${tempfile}  ${shaderfile}
		MAIN_DEPENDENCY ${shaderfile}
		DEPENDS ${dependencies}
	)
endfunction(shader_compile)

foreach(shader ${UGS_SHADER_FILES})
	shader_compile(${shader} "${UGS_DEP_SHADER_FILES}")
endforeach()

#######################################################################################
# custom-defined function to set target property
# reference https://cmake.org/cmake/help/v3.0/manual/cmake-properties.7.html 
#######################################################################################
function(my_default_target_property target_name)
	#message(STATUS "ARGC=${ARGC} target_name='${target_name}' ")
	if (NOT ${ARGC} EQUAL 1)
	message(STATUS "ARGC is NOT expected.") 
	endif()
	if (NOT ${ARGV0} STREQUAL ${target_name})
	message(STATUS "ARGV0 is NOT expected. ARGV0='${ARGV0}' target_name='${target_name}'") 
	endif()  
	set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${UGS_BINARY_DIR})
	set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${UGS_BINARY_DIR})
	set_target_properties(${target_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${UGS_BINARY_DIR})
	set_target_properties(${target_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${UGS_BINARY_DIR})
	set_target_properties(${target_name} PROPERTIES DEBUG_POSTFIX "_d")
	set_target_properties(${target_name} PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
endfunction()

#######################################################################################
## preprocess macro definition
#######################################################################################
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DBX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1)
add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
add_definitions(-DUSE_DL_PREFIX)

if(MSVC)
	add_definitions(-DWIN32)
	add_definitions(-D_WIN32)
	add_definitions(-DUNICODE)
	add_definitions(-D_UNICODE)
	#add_definitions(-DNOMINMAX)
endif(MSVC)


if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

find_package(DirectX)

if(VERBOSE_MODE)
	message(STATUS "${PROJECT_NAME} CMAKE_COMMAND='${CMAKE_COMMAND}' ")
	message(STATUS "${PROJECT_NAME} CMAKE_CURRENT_BINARY_DIR='${CMAKE_CURRENT_BINARY_DIR}' ")
	message(STATUS "${PROJECT_NAME} LINK_OPTIONS =   '${LINK_OPTIONS}' ")
	message(STATUS "${PROJECT_NAME} CMAKE_CXX_FLAGS ='${CMAKE_CXX_FLAGS}' ")
endif(VERBOSE_MODE)

#######################################################################################
# ugs libraries and executables
#######################################################################################
add_library( ugs STATIC  ${UGS_FILES} ${UGS_INCLUDES}  ${UGS_EXT_FILES} ${UGS_SHADER_FILES})
if (WIN32)
	set_target_properties ( ugs PROPERTIES OUTPUT_NAME libugs ) #Avoid a name clash when building on Visual Studio
endif()

set(ALL_UGS_LIBS
	ugs
	${DirectX_D3D11_LIBRARY} 
	${DirectX_D3D11_COMPILER} 
	${CMAKE_CURRENT_SOURCE_DIR}/framework/thirdparty/Assimp/lib/assimp-vc141-mt.lib
	CACHE STRING "All libs UGS required." FORCE
)

add_custom_command(TARGET ugs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/framework/thirdparty/Assimp/lib/assimp-vc141-mt.dll ${UGS_BINARY_DIR}/
)


add_custom_command(TARGET ugs POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${UGS_BINARY_DIR}/shaders/
)

#######################################################################################
# Demos based on ugs
#######################################################################################
if( UGS_BUILD_SAMPLES)
	set(EXE_NAME "SimpleInstanceDraw")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/SimpleInstanceDraw.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})
	
	set(EXE_NAME "CubeMapViewer")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/CubeMapViewer.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})
	
	set(EXE_NAME "CubeMap")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/CubeMap.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})

	set(EXE_NAME "TranslateAnimation")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/TranslateAnimation.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})


	set(EXE_NAME "ModelViewer")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/ModelViewer.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})


	set(EXE_NAME "Canvas")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/Canvas.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})

	set(EXE_NAME "Sphere")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/Sphere.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})


	set(EXE_NAME "Fractal")
	ADD_EXECUTABLE ( ${EXE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/examples/Fractal.cpp)
	TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_UGS_LIBS} )
	my_default_target_property(${EXE_NAME})
endif(UGS_BUILD_SAMPLES)

#######################################################################################
# Hide the console window in visual studio projects
#######################################################################################
if(ENABLE_HIDECONSOLE_BUILD)
	if(MSVC)
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
		set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
		set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
		message(STATUS "Hide console on windows msvc.")
	endif()
endif()
