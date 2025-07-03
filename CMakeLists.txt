cmake_minimum_required(VERSION 3.10)
project(SensorViz)

# C++17 standardını kullan
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug modunda daha iyi hata ayıklama için
set(CMAKE_BUILD_TYPE Debug)

# Include dizinlerini belirle
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/libs/glfw/include)

# GLFW kütüphane dizinini belirle
link_directories(${CMAKE_SOURCE_DIR}/libs/glfw/lib)

# Kaynak dosyaları
set(SOURCES
    src/main.cpp
    src/glad.c
)

# Executable oluştur
add_executable(${PROJECT_NAME} ${SOURCES})

# GLFW'yi linkle (MinGW için) - static linking
target_link_libraries(${PROJECT_NAME} 
    ${CMAKE_SOURCE_DIR}/libs/glfw/lib/libglfw3.a
    opengl32
    gdi32
    winmm
    user32
    kernel32
    comdlg32
)

# Windows specific ayarlar
if(WIN32)
    target_link_libraries(${PROJECT_NAME} 
        ws2_32
        ole32
        shell32
        advapi32
        psapi
        imm32
        version
        setupapi
        xinput9_1_0
    )
endif()

# GLFW DLL'ini build dizinine kopyala
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/libs/glfw/lib/glfw3.dll"
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

# Compiler-specific ayarlar
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

message(STATUS "SensorViz projesi yapılandırıldı")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}") 