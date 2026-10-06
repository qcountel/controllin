#include "pch.h"
#include "taskBarIcon.h"
#include "cMain.h"
#include "cApp.h"

wxBEGIN_EVENT_TABLE(taskBarIcon, wxTaskBarIcon)
EVT_MENU(wxID_CLOSE, taskBarIcon::onCloseMenu)
EVT_MENU(301, taskBarIcon::onInjectMenu)
EVT_MENU(302, taskBarIcon::onOpenMenu)
EVT_TASKBAR_LEFT_DCLICK(taskBarIcon::onTaskBarDClick)
wxEND_EVENT_TABLE();

wxMenu *taskBarIcon::CreatePopupMenu() {
    menu = new wxMenu();
    menu->Append(301, "Play");
    menu->Append(302, "Open");
    menu->Append(wxID_CLOSE);
    return menu;
}

void taskBarIcon::onTaskBarDClick(wxTaskBarIconEvent &evt) {
    CMAIN_INSTANCE->Show();
    evt.Skip();
}

void taskBarIcon::onInjectMenu(wxCommandEvent &evt) {
    // Trigger the same code path as pressing the PLAY button.
    wxCommandEvent playEvt(wxEVT_BUTTON, 102);
    CMAIN_INSTANCE->OnPlayButton(playEvt);
    evt.Skip();
}

void taskBarIcon::onCloseMenu(wxCommandEvent &evt) {
    CMAIN_INSTANCE->Close();
    evt.Skip();
}

void taskBarIcon::onOpenMenu(wxCommandEvent &evt) {
    CMAIN_INSTANCE->Show();
    evt.Skip();
}
