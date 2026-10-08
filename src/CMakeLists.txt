target_sources(${PROJECT_NAME} PRIVATE cApp.cpp
cMain.cpp
flatButton.cpp
launcher.cpp
FixFilePerms.cpp
inject.cpp
pch.cpp
taskBarIcon.cpp
icon.rc)

target_precompile_headers(${PROJECT_NAME} PRIVATE "pch.h")

# The app icon is stored in git as base64 (icon/icon.ico.b64) and decoded here,
# unless a real icon/icon.ico is already present.
set(ICON_ICO "${CMAKE_CURRENT_SOURCE_DIR}/icon/icon.ico")
set(ICON_B64 "${CMAKE_CURRENT_SOURCE_DIR}/icon/icon.ico.b64")
if(NOT EXISTS "${ICON_ICO}" AND EXISTS "${ICON_B64}")
    file(TO_NATIVE_PATH "${ICON_B64}" ICON_B64_NATIVE)
    file(TO_NATIVE_PATH "${ICON_ICO}" ICON_ICO_NATIVE)
    execute_process(COMMAND certutil -f -decode "${ICON_B64_NATIVE}" "${ICON_ICO_NATIVE}"
                    RESULT_VARIABLE ICON_DECODE_RESULT OUTPUT_QUIET)
    if(NOT ICON_DECODE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to decode src/icon/icon.ico.b64")
    endif()
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ICON_B64}")
