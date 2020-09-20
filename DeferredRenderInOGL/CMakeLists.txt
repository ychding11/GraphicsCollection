cmake_minimum_required (VERSION 3.6)

set(PROJECT_NAME OGLRenderer)

project (${PROJECT_NAME})

message(STATUS "bigin build project '${PROJECT_NAME}' ")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

set(UGS_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin CACHE STRING "Binary Dir" FORCE) #force update even if it in cache
message(STATUS "UGS_BINARY_DIR='${UGS_BINARY_DIR}' ")

set(_UGS_LOCATION_ ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "UGS Location" FORCE)
set(_UGS_SHADER_LOCATION_ ${CMAKE_CURRENT_SOURCE_DIR}/shaders/ CACHE STRING "UGS Shader Location" FORCE)


add_compile_definitions(UGS_LOCATION="${_UGS_LOCATION_}")
add_compile_definitions(UGS_SHADER_LOCATION="${_UGS_SHADER_LOCATION_}")


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


file(GLOB SHADER_FILES

)

file(GLOB SRC_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/3party/*.h
	${CMAKE_CURRENT_SOURCE_DIR}/3party/*.cpp
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/
  ${CMAKE_CURRENT_SOURCE_DIR}/3party
  ${CMAKE_CURRENT_SOURCE_DIR}/3party/FreeImage
  ${CMAKE_CURRENT_SOURCE_DIR}/3party/freeglut/include
  ${CMAKE_CURRENT_SOURCE_DIR}/3party/glew/include
  ${CMAKE_CURRENT_SOURCE_DIR}/3party/glm
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

group_files("${SRC_FILES}")


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

if(VERBOSE_MODE)
	message(STATUS "${PROJECT_NAME} CMAKE_COMMAND='${CMAKE_COMMAND}' ")
	message(STATUS "${PROJECT_NAME} CMAKE_CURRENT_BINARY_DIR='${CMAKE_CURRENT_BINARY_DIR}' ")
	message(STATUS "${PROJECT_NAME} LINK_OPTIONS =   '${LINK_OPTIONS}' ")
	message(STATUS "${PROJECT_NAME} CMAKE_CXX_FLAGS ='${CMAKE_CXX_FLAGS}' ")
endif(VERBOSE_MODE)

#######################################################################################
# ugs libraries and executables
#######################################################################################
set(ALL_LIBS
	${CMAKE_CURRENT_SOURCE_DIR}/3party/FreeImage/FreeImage.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3party/freeglut/lib/freeglut.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/3party/glew/lib/glew32.lib
	CACHE STRING "All depnedency libs."  FORCE
)

set(EXE_NAME "OGLDefered")
ADD_EXECUTABLE( ${EXE_NAME}  ${SRC_FILES})
TARGET_LINK_LIBRARIES ( ${EXE_NAME} ${ALL_LIBS} )
my_default_target_property(${EXE_NAME})


#######################################################################################
# Hide the console window in visual studio projects
#######################################################################################

if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
	set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
	set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
	message(STATUS "Hide console on windows msvc.")
endif()

