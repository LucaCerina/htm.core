# -----------------------------------------------------------------------------
# HTM Community Edition of NuPIC
# Copyright (C) 2016, Numenta, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
# -----------------------------------------------------------------------------
# Note: yaml-cpp includes an older version of gtest so 
#       turn off YAML_CPP_BUILD_TESTS to prevent it from building gtest.
message(STATUS "${REPOSITORY_DIR}/build/ThirdParty/share/yaml-cpp.zip")
if(EXISTS "${REPOSITORY_DIR}/build/ThirdParty/share/yaml-cpp.zip")
    set(URL "${REPOSITORY_DIR}/build/ThirdParty/share/yaml-cpp.zip")
else()
    # There seems to be something wrong with the 0.6.2 distribution.  Use the v0.6.3.
    set(URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.3.zip)
endif()

message(STATUS "Obtaining yaml-cpp from ${URL}" )
include(DownloadProject/DownloadProject.cmake)
download_project(PROJ yaml-cpp
	PREFIX ${EP_BASE}/yaml-cpp
	URL ${URL}
	UPDATE_DISCONNECTED 1
	QUIET
	)
set(YAML_CPP_INSTALL OFF CACHE BOOL "prevent install, not needed." FORCE) 
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "prevent tools from being build" FORCE) 
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "prevent gtest from being build" FORCE) 
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "prevent contrib modules" FORCE) 
set(MSVC_SHARED_RT ON CACHE BOOL "Use compile option /MD rather than /MT" FORCE)
add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})

set(yaml_INCLUDE_DIRS ${yaml-cpp_SOURCE_DIR}/include) 
if (MSVC)
  set(yaml_LIBRARIES   "${yaml-cpp_BINARY_DIR}$<$<CONFIG:Release>:/Release/libyaml-cppmd.lib>$<$<CONFIG:Debug>:/Debug/libyaml-cppmdd.lib>") 
else()
  set(yaml_LIBRARIES   ${yaml-cpp_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}) 
endif()
FILE(APPEND "${EXPORT_FILE_NAME}" "yaml_INCLUDE_DIRS@@@${yaml_INCLUDE_DIRS}\n")
FILE(APPEND "${EXPORT_FILE_NAME}" "yaml_LIBRARIES@@@${yaml_LIBRARIES}\n")
FILE(APPEND "${EXPORT_FILE_NAME}" "yaml_DEFINE@@@YAML_PARSER_yamlcpp\n")

