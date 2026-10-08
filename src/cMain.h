#pragma once

#include "config.h"
#include "taskBarIcon.h"
#include "flatButton.h"

#include <thread>
#include <atomic>

// Custom event used to deliver status text from the worker thread to the UI thread.
wxDECLARE_EVENT(EVT_PLAY_STATUS, wxThreadEvent);

class cMain : public wxFrame {
public:
    // Header: PLAY tab + "Controllin +" (subscription) button
    FlatButton*   tab_Play = nullptr;
    FlatButton*   tab_Plus = nullptr;

    // Play page
    wxPanel*      pagePlay = nullptr;
    FlatButton*   btn_Play = nullptr;
    wxStaticText* lbl_Status = nullptr;

    std::thread worker;
    std::atomic<bool> busy{ false };

    cMain();
    virtual ~cMain() override;

    // Header
    void OnPlus(wxCommandEvent& evt);

    // Play
    void OnPlayButton(wxCommandEvent& evt);
    void OnPlayStatus(wxThreadEvent& evt);
    void PlayWorker();
    void postStatus(const std::wstring& msg);

    // Painting of the grey emblem on the Play page.
    void OnPlayPagePaint(wxPaintEvent& evt);

    void layoutTabs();

    wxDECLARE_EVENT_TABLE();
};
