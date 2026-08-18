file(READ "${CMAKE_CURRENT_LIST_DIR}/pack.json" PACK_JSON)
string(JSON PACK_NAME GET "${PACK_JSON}" name)

# Leaf pack: only rigComponent. No other RigKit pack may depend on this.
if(NOT TARGET rigComponent)
	if(DEFINED RIGKIT_ROOT AND NOT RIGKIT_ROOT STREQUAL "")
		set(_rigComponent_dir "${RIGKIT_ROOT}/packs/rigComponent")
	else()
		set(_rigComponent_dir "${CMAKE_SOURCE_DIR}/packs/rigComponent")
	endif()
	if(EXISTS "${_rigComponent_dir}/CMakeLists.txt")
		add_subdirectory("${_rigComponent_dir}" "${CMAKE_BINARY_DIR}/packs/rigComponent")
	else()
		message(FATAL_ERROR "rigAssimp requires rigComponent under packs/rigComponent")
	endif()
endif()

# Assimp: prefer system / vcpkg; else FetchContent (only when an app pulls this pack).
set(_rigAssimp_assimp_target "")
find_package(assimp CONFIG QUIET)
if(TARGET assimp::assimp)
	set(_rigAssimp_assimp_target assimp::assimp)
elseif(assimp_FOUND)
	set(_rigAssimp_assimp_target assimp::assimp)
endif()
if(NOT _rigAssimp_assimp_target)
	find_package(Assimp QUIET)
	if(TARGET assimp::assimp)
		set(_rigAssimp_assimp_target assimp::assimp)
	elseif(ASSIMP_LIBRARIES)
		add_library(rigAssimp_assimp_external INTERFACE)
		target_include_directories(rigAssimp_assimp_external INTERFACE ${ASSIMP_INCLUDE_DIRS})
		target_link_libraries(rigAssimp_assimp_external INTERFACE ${ASSIMP_LIBRARIES})
		set(_rigAssimp_assimp_target rigAssimp_assimp_external)
	endif()
endif()

if(NOT _rigAssimp_assimp_target)
	include(FetchContent)
	set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
	set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
	set(ASSIMP_INJECT_DEBUG_POSTPROCESSORS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
	set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_COLLADA_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_STL_IMPORTER ON CACHE BOOL "" FORCE)
	set(ASSIMP_BUILD_PLY_IMPORTER ON CACHE BOOL "" FORCE)
	FetchContent_Declare(
		assimp
		GIT_REPOSITORY https://github.com/assimp/assimp.git
		GIT_TAG v5.4.3
		GIT_SHALLOW TRUE
	)
	FetchContent_MakeAvailable(assimp)
	if(TARGET assimp::assimp)
		set(_rigAssimp_assimp_target assimp::assimp)
	elseif(TARGET assimp)
		set(_rigAssimp_assimp_target assimp)
	else()
		message(FATAL_ERROR "rigAssimp: Assimp FetchContent did not create an assimp target")
	endif()
	message(STATUS "Configured pack: ${PACK_NAME} (Assimp via FetchContent v5.4.3)")
else()
	message(STATUS "Configured pack: ${PACK_NAME} (Assimp via find_package)")
endif()

add_library(${PACK_NAME} STATIC
	src/rigAssimp.cpp
	src/rigAssimp.h
	src/AssimpIO.cpp
	src/AssimpIO.h
)
target_link_libraries(${PACK_NAME} PUBLIC rigkit rigComponent ${_rigAssimp_assimp_target})
target_include_directories(${PACK_NAME} PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src>
)

if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/tools/assimp_io_smoke.cpp")
	add_executable(assimp_io_smoke "${CMAKE_CURRENT_LIST_DIR}/tools/assimp_io_smoke.cpp")
	target_link_libraries(assimp_io_smoke PRIVATE ${PACK_NAME})
	# Default sample for local/CI smoke when run without args from pack tree.
	target_compile_definitions(assimp_io_smoke PRIVATE
		RIGASSIMP_SAMPLE_STL="${CMAKE_CURRENT_LIST_DIR}/examples/load/data/prism.stl")
endif()

message(STATUS "Configured pack: ${PACK_NAME} (optional Assimp IO — leaf; nothing depends on it)")
