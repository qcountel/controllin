#pragma once

#include "config.h"
#include "taskBarIcon.h"
#include "flatButton.h"

#include <wx/radiobut.h>
#include <thread>
#include <atomic>

// Custom event used to deliver status text from the worker thread to the UI thread.
wxDECLARE_EVENT(EVT_PLAY_STATUS, wxThreadEvent);

class cMain : public wxFrame {
public:
    // Tab headers
    FlatButton*   tab_Play = nullptr;
    FlatButton*   tab_Settings = nullptr;

    // Pages
    wxPanel*      pagePlay = nullptr;
    wxPanel*      pageSettings = nullptr;

    // Play page
    FlatButton*   btn_Play = nullptr;
    wxStaticText* lbl_Status = nullptr;

    // Settings page
    wxRadioButton* radio_Local = nullptr;
    wxRadioButton* radio_Github = nullptr;
    wxTextCtrl*    txt_LocalPath = nullptr;
    FlatButton*    btn_Browse = nullptr;
    wxTextCtrl*    txt_McPath = nullptr;
    FlatButton*    btn_Save = nullptr;

    Config cfg;

    std::thread worker;
    std::atomic<bool> busy{ false };

    cMain();
    virtual ~cMain() override;

    // Navigation
    void ShowPage(bool settings);
    void OnTabPlay(wxCommandEvent& evt);
    void OnTabSettings(wxCommandEvent& evt);

    // Play
    void OnPlayButton(wxCommandEvent& evt);
    void OnPlayStatus(wxThreadEvent& evt);
    void PlayWorker();
    void postStatus(const std::wstring& msg);

    // Settings
    void OnBrowse(wxCommandEvent& evt);
    void OnSave(wxCommandEvent& evt);
    void OnSourceChanged(wxCommandEvent& evt);
    void refreshEnabledState();

    // Painting of the grey emblem on the Play page.
    void OnPlayPagePaint(wxPaintEvent& evt);

    void layoutTabs();

    wxDECLARE_EVENT_TABLE();
};