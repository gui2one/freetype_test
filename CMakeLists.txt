cmake_minimum_required(VERSION 3.2)

project(freetype_test)

link_directories(

    vendor/msdfgen/Release
)

include_directories(
    vendor/freetype/include
    vendor/msdfgen
    vendor/msdfgen/core
    vendor/
)
add_executable(${PROJECT_NAME}

    src/main.cpp
)

#freetype
add_subdirectory(vendor/freetype)
target_include_directories(${PROJECT_NAME} PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} freetype ${FREETYPE_LIBRARY})

# tinyxml2
add_subdirectory(vendor/tinyxml2)
target_include_directories(${PROJECT_NAME} PUBLIC ${tinyxml2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} tinyxml2 ${tinyxml2_LIBRARY})

# libPNG
add_subdirectory(vendor/libpng)
target_include_directories(${PROJECT_NAME} PUBLIC ${libpng_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} libpng ${libpng_LIBRARY})

# # msdfgen

set(FREETYPE_INCLUDE_DIRS vendor/freetype/include)
set(FREETYPE_LIBRARY freetype.lib)

# option(MSDFGEN_CORE_ONLY "Only build the core library with no dependencies" ON)
# option(MSDFGEN_BUILD_STANDALONE "Build the msdfgen standalone executable" OFF)
option(MSDFGEN_USE_VCPKG "Use vcpkg package manager to link project dependencies" OFF)
# option(MSDFGEN_USE_OPENMP "Build with OpenMP support for multithreaded code" OFF)
# option(MSDFGEN_USE_CPP11 "Build with C++11 enabled" OFF)
# option(MSDFGEN_USE_SKIA "Build with the Skia library" OFF)
# option(MSDFGEN_INSTALL "Generate installation target" OFF)
# option(MSDFGEN_DYNAMIC_RUNTIME "Link dynamic runtime library instead of static" OFF)
# option(BUILD_SHARED_LIBS "Generate dynamic library files instead of static" OFF)


add_subdirectory(vendor/msdfgen)
target_include_directories(${PROJECT_NAME} PUBLIC ${MSDFGEN_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
    msdfgen-core
    # Freetype::Freetype
)




