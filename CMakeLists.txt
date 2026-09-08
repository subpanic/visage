cmake_minimum_required(VERSION 3.17)
set(CMAKE_POLICY_DEFAULT_CMP0177 NEW)

if (APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version")
endif ()

if (NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif ()

project(visage VERSION 0.1.0)

# Ensure PIC when visage is built as a subdirectory (needed for Linux plugin shared libraries)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif ()

option(VISAGE_AMALGAMATED_BUILD "Compile sources together" ON)
option(VISAGE_ENABLE_WIDGETS "Add widgets library" ON)
option(VISAGE_ENABLE_BACKGROUND_GRAPHICS_THREAD "Offloads graphics rendering to a background thread" OFF)
option(VISAGE_ENABLE_GRAPHICS_DEBUG_LOGGING "Shows graphics debug log in console in debug mode" OFF)
option(VISAGE_ENABLE_DRAW_METRICS "Enable frame draw time instrumentation and reporting" OFF)
option(VISAGE_ADDRESS_SANITIZER "Enable AddressSanitizer" OFF)

if (VISAGE_ADDRESS_SANITIZER)
  if (MSVC)
    add_compile_options(/fsanitize=address /Zi)
    add_link_options(/fsanitize=address)
  else ()
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
  endif ()
endif ()

if (VISAGE_ENABLE_DRAW_METRICS)
  message(STATUS "VISAGE: Enabling draw metrics")
  add_compile_definitions(VISAGE_DRAW_METRICS=1)
endif ()

set(VISAGE_APPLICATION_NAME ${CMAKE_PROJECT_NAME} CACHE STRING "Application name used for default window title")

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  option(VISAGE_BUILD_EXAMPLES "Build examples" ON)
  option(VISAGE_BUILD_TESTS "Build tests" ON)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
else ()
  option(VISAGE_BUILD_EXAMPLES "Build examples" OFF)
  option(VISAGE_BUILD_TESTS "Build tests" OFF)
endif ()

set(VISAGE_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

# Shared CPM source cache: dependencies (bgfx, freetype) download once here
# and are reused by all build trees/presets. Override with
# -DCPM_SOURCE_CACHE=... or the CPM_SOURCE_CACHE env var.
if (NOT DEFINED CPM_SOURCE_CACHE AND NOT DEFINED ENV{CPM_SOURCE_CACHE})
  if (WIN32)
    set(_cpm_cache_default "$ENV{LOCALAPPDATA}/CPM")
  elseif (UNIX AND NOT APPLE AND DEFINED ENV{XDG_CACHE_HOME})
    set(_cpm_cache_default "$ENV{XDG_CACHE_HOME}/CPM")
  else ()
    set(_cpm_cache_default "$ENV{HOME}/.cache/CPM")
  endif ()
  set(CPM_SOURCE_CACHE "${_cpm_cache_default}" CACHE PATH "Shared CPM source cache")
endif ()

include(FetchContent)
include(cmake/CPM.cmake)
include(cmake/compile_flags.cmake)
include(cmake/testing_framework.cmake)

set(VISAGE_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)

function(amalgamate_headers AMALGAMATED_HEADER_NAME FILE_HEADERS)
  set(FILE_CONTENTS "// Auto generated amalgamated header.\n#pragma once\n")
  foreach (FILE ${FILE_HEADERS})
    set(FILE_CONTENTS "${FILE_CONTENTS}#include \"${FILE}\"\n")
  endforeach ()
  file(CONFIGURE OUTPUT "${AMALGAMATED_HEADER_NAME}" CONTENT "${FILE_CONTENTS}")
endfunction()

function(amalgamate_sources GENERATED_FILES AMALGAMATED_SOURCE_BASE FILE_SOURCES)
  set(IDX 0)
  set(PART 0)
  set(GENERATED "")

  foreach (FILE ${FILE_SOURCES})
    set_source_files_properties("${FILE}" PROPERTIES HEADER_FILE_ONLY TRUE)
    if (IDX EQUAL 0)
      if (PART GREATER 0)
        file(CONFIGURE OUTPUT "${AMALGAMATED_SOURCE_BASE}${PART}.cpp" CONTENT "${FILE_CONTENTS}")
      endif ()

      math(EXPR PART "${PART} + 1")
      list(APPEND GENERATED "${AMALGAMATED_SOURCE_BASE}${PART}.cpp")
      set(FILE_CONTENTS "// Auto generated amalgamated source.\n")
    endif ()
    set(FILE_CONTENTS "${FILE_CONTENTS}#include \"${FILE}\"\n")

    math(EXPR IDX "${IDX} + 1")
    if (IDX GREATER 6)
      set(IDX 0)
    endif ()
  endforeach ()

  set(${GENERATED_FILES} "${GENERATED}" PARENT_SCOPE)
  file(CONFIGURE OUTPUT "${AMALGAMATED_SOURCE_BASE}${PART}.cpp" CONTENT "${FILE_CONTENTS}")
endfunction()

add_subdirectory(visage_file_embed)
add_subdirectory(visage_graphics)
add_subdirectory(visage_ui)
add_subdirectory(visage_utils)
add_subdirectory(visage_widgets)
add_subdirectory(visage_windowing)
add_subdirectory(visage_app)

set(DUMMY_FILE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)
if (NOT EXISTS ${DUMMY_FILE})
  file(WRITE ${DUMMY_FILE} "")
endif ()

add_library(visage STATIC
  ${DUMMY_FILE}
)

target_include_directories(visage SYSTEM PUBLIC ${VISAGE_INCLUDE_PATH} ${VISAGE_INCLUDE})
target_link_libraries(visage PRIVATE
  VisageApp
  VisageGraphics
  VisageUtils
  VisageUi
  VisageWidgets
  VisageWindowing
)

set_target_properties(visage PROPERTIES FOLDER "visage")

if (VISAGE_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif ()
