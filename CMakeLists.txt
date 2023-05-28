cmake_minimum_required(VERSION 3.2)

project(freetype_test)

link_directories(

)

include_directories(
    vendor/glm
    vendor/
)
add_executable(${PROJECT_NAME}

    src/main.cpp
    src/GlyphShape.h
)


#freetype
add_subdirectory(vendor/freetype)
target_include_directories(${PROJECT_NAME} PUBLIC) # ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} freetype ) # ${FREETYPE_LIBRARY})

#poly2tri
add_subdirectory(vendor/poly2tri)
target_include_directories(${PROJECT_NAME} PUBLIC) # ${POLY2TRY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} poly2tri ) # ${POLY2TRY_LIBRARY})


# tiny-utf8
add_subdirectory(vendor/tiny-utf8)
target_include_directories(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} tinyutf8)
