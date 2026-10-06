target_sources(${PROJECT_NAME} PRIVATE cApp.cpp
cMain.cpp
flatButton.cpp
config.cpp
launcher.cpp
FixFilePerms.cpp
inject.cpp
pch.cpp
taskBarIcon.cpp
icon.rc)

target_precompile_headers(${PROJECT_NAME} PRIVATE "pch.h")
