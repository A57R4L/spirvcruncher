cmake_minimum_required (VERSION 3.8)
# set(CMAKE_VERBOSE_MAKEFILE ON)

project("spirvcruncher" VERSION 0.1.0 DESCRIPTION "Packer tool for spir-v binaries" LANGUAGES C CXX)

# Add CPM.cmake for dependencies
set(CPM_DOWNLOAD_VERSION 0.42.0)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Getting CPM.cmake...")
    file(DOWNLOAD https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()
include(${CPM_DOWNLOAD_LOCATION})

# Include fork of smol-v

CPMAddPackage(
  NAME smol
  GITHUB_REPOSITORY A57R4L/smol-v
  GIT_TAG spirvcruncher
)

set(SMOL_SOURCES
	${smol_SOURCE_DIR}/source/smolv.cpp
	${smol_SOURCE_DIR}/source/smolv.h
	)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/src
		${smol_SOURCE_DIR}/source
		${CMAKE_BINARY_DIR}
)

# Generate embeddable source from data file
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/generated_shadertemplate.h
    COMMAND ${CMAKE_COMMAND} -DINPUT=${CMAKE_SOURCE_DIR}/data/spirvcruncher_template.h
                              -DOUTPUT=${CMAKE_BINARY_DIR}/generated_shadertemplate.h
                              -P ${CMAKE_SOURCE_DIR}/embed_shadercode.cmake
    DEPENDS ${CMAKE_SOURCE_DIR}/data/spirvcruncher_template.h
)

# The custom target to make sure the header is generated
add_custom_target(generate_shadertemplate DEPENDS ${CMAKE_BINARY_DIR}/generated_shadertemplate.h)

# Add source
add_executable(spirvcruncher src/spirvcruncher.cpp ${smol_SOURCE_DIR}/source/smolv.cpp ${CMAKE_BINARY_DIR}/generated_shadertemplate.h)
add_dependencies(spirvcruncher generate_shadertemplate)

# Add dependency to generated template

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET spirvcruncher PROPERTY CXX_STANDARD 20)
endif()

# Copy templates to exe directory
file(GLOB TEMPLATE_FILES "${CMAKE_SOURCE_DIR}/data/*.*")
add_custom_command(
  TARGET spirvcruncher POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${TEMPLATE_FILES} $<TARGET_FILE_DIR:spirvcruncher>
)
