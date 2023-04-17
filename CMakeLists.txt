cmake_minimum_required(VERSION 3.1)

project(ChaosRaytracing2023)

##############
### CONFIG ###
##############

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/install)



###############
### LIBRARY ###
###############

set(TARGET_LIB_NAME renderer_lib)

set(LIB_HEADERS
    include/lib_export.h
    include/renderer_lib.h
    include/utils.h
    include/vector.h
    include/matrix.h
    include/camera.h
    include/scene_object.h
)

set(LIB_SOURCES
    src/lib_export.cpp
    src/renderer_lib.cpp
    src/scene_object.cpp
)

add_library(${TARGET_LIB_NAME} SHARED "${LIB_SOURCES};${LIB_HEADERS}")
target_compile_features(${TARGET_LIB_NAME} PRIVATE cxx_std_17)
target_include_directories(${TARGET_LIB_NAME} PUBLIC include)
target_compile_definitions(${TARGET_LIB_NAME} PRIVATE ChaosRendererEXPORTS)

install(TARGETS ${TARGET_LIB_NAME} DESTINATION lib)

# Install lib headers when needed - currently in a shared folder with app
#install(FILES ${LIB_HEADERS} DESTINATION include/${TARGET_LIB_NAME})

option(SETUP_PYTHON_ENV "Whether to setup the Python environment. Python 3 with tkinter required." ON)
if (SETUP_PYTHON_ENV)
    set(PYTHON_EXECUTABLE "python.exe" CACHE PATH "Path to the Python executable ")
    set(VENV_PATH "${CMAKE_BINARY_DIR}/.venv")
    execute_process(COMMAND ${PYTHON_EXECUTABLE} "-m" "venv" "${VENV_PATH}")
    execute_process(COMMAND "${VENV_PATH}/Scripts/pip" "install" "pillow" "customtkinter" "CTkColorPicker" "numpy")
endif()


# Add a way to start the Python GUI directly from VS
if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    set_target_properties(${TARGET_LIB_NAME} PROPERTIES
        VS_DEBUGGER_COMMAND "${PYTHON_EXECUTABLE}"
        VS_DEBUGGER_COMMAND_ARGUMENTS "${CMAKE_CURRENT_SOURCE_DIR}\\python-gui\\gui.py"
        VS_DEBUGGER_ENVIRONMENT "CHAOS_RAYTRACING_LIB_PATH=$<TARGET_FILE_DIR:${TARGET_LIB_NAME}>\nVIRTUAL_ENV=${VENV_PATH}\nPYTHONPATH=${VENV_PATH}\\Lib\\site-packages;\"%PYTHONPATH%\""
    )
endif()


###########
### APP ###
###########

set(APP_HEADERS
    include/lib_export.h
    include/gui_win.hpp
    include/utils.h
)

set(APP_SOURCES
    src/renderer_app.cpp
)

add_executable(renderer_app "${APP_SOURCES};${APP_HEADERS}")
target_link_libraries(renderer_app PRIVATE ${TARGET_LIB_NAME})
target_include_directories(renderer_app PUBLIC include)

install(TARGETS renderer_app DESTINATION bin)
