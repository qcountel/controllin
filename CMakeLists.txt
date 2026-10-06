
cmake_minimum_required(VERSION 3.20)
project(ControllinInjector VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wxBUILD_SHARED OFF CACHE BOOL "OFF for static library!" FORCE) # force wxWidgets to use static library
mark_as_advanced(wxBUILD_SHARED)

add_executable(${PROJECT_NAME} WIN32)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Controllin Injector")
add_subdirectory(src)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'") # requireAdministrator

add_subdirectory(wxWidgets-3.2.0)

target_link_libraries(${PROJECT_NAME} wx::net wx::core wx::base wininet shell32 dwmapi)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})