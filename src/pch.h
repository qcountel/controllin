#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <wx/wx.h>
#include <wx/taskbar.h>
#include <wx/notifmsg.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <wx/mstream.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

#include <windows.h>
#include <TlHelp32.h>

#include <fstream>
#include <string>
#include <algorithm>
#include <thread>
#include <atomic>

#include "cApp.h"
#include "cMain.h"
#include "config.h"
#include "FixFilePerms.h"
#include "inject.h"
#include "launcher.h"

#include "icon/icon.xpm"
