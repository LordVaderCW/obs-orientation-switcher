cmake_minimum_required(VERSION 3.28)

project(obs-orientation-switcher VERSION 1.0.1 LANGUAGES CXX)

find_package(libobs CONFIG REQUIRED)
find_package(obs-frontend-api CONFIG REQUIRED)

add_library(${PROJECT_NAME} MODULE src/plugin-main.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE OBS::libobs OBS::obs-frontend-api)

set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "obs-orientation-switcher"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "obs-plugins/64bit"
    LIBRARY DESTINATION "obs-plugins/64bit"
)
