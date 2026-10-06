#include "pch.h"
#include "cMain.h"
#include "launcher.h"
#include "inject.h"
#include "theme.h"

enum {
    ID_TAB_PLAY = 101,
    ID_TAB_SETTINGS = 102,
    ID_PLAY = 103,
    ID_RADIO_LOCAL = 104,
    ID_RADIO_GITHUB = 105,
    ID_BROWSE = 106,
    ID_SAVE = 107,
};

static const int TAB_H = 42;

wxDEFINE_EVENT(EVT_PLAY_STATUS, wxThreadEvent);

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(ID_TAB_PLAY, cMain::OnTabPlay)
EVT_BUTTON(ID_TAB_SETTINGS, cMain::OnTabSettings)
EVT_BUTTON(ID_PLAY, cMain::OnPlayButton)
EVT_BUTTON(ID_BROWSE, cMain::OnBrowse)
EVT_BUTTON(ID_SAVE, cMain::OnSave)
EVT_RADIOBUTTON(ID_RADIO_LOCAL, cMain::OnSourceChanged)
EVT_RADIOBUTTON(ID_RADIO_GITHUB, cMain::OnSourceChanged)
wxEND_EVENT_TABLE();

static void StyleInput(wxTextCtrl* t) {
    t->SetBackgroundColour(Theme::INPUT_BG);
    t->SetForegroundColour(Theme::FG);
    t->SetFont(Theme::Font(10));
}
static void StyleRadio(wxRadioButton* r) {
    r->SetForegroundColour(Theme::FG);
    r->SetBackgroundColour(Theme::CARD);
    r->SetFont(Theme::Font(10));
}
static wxStaticText* MakeLabel(wxWindow* parent, const wxString& text, wxColour bg,
                               bool dim = false) {
    wxStaticText* s = new wxStaticText(parent, wxID_ANY, text);
    s->SetForegroundColour(dim ? Theme::FG_DIM : Theme::FG);
    s->SetBackgroundColour(bg);
    s->SetFont(Theme::Font(dim ? 8 : 10));
    return s;
}

cMain::cMain()
    : wxFrame(nullptr, wxID_ANY, L"Controllin Injector", wxDefaultPosition, wxSize(400, 600),
              wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN) {

    Theme::EnsurePixelFont();

    if (!this->cfg.serializeConfig()) {
        this->cfg.updateConfigFile();
    }

    wxIcon icon(icon_xpm);
    this->SetIcon(icon);
    Theme::ApplyDarkTitleBar((HWND)this->GetHandle());
    this->SetBackgroundColour(Theme::BG);

    // ---- Tab headers ----
    this->tab_Play = new FlatButton(this, ID_TAB_PLAY, L"PLAY");
    this->tab_Play->SetButtonStyle(FlatButton::STYLE_TAB);
    this->tab_Play->SetFont(Theme::Font(10));
    this->tab_Play->SetSelected(true);

    this->tab_Settings = new FlatButton(this, ID_TAB_SETTINGS, L"SETTINGS");
    this->tab_Settings->SetButtonStyle(FlatButton::STYLE_TAB);
    this->tab_Settings->SetFont(Theme::Font(10));

    // =================== PLAY PAGE ===================
    this->pagePlay = new wxPanel(this, wxID_ANY);
    this->pagePlay->SetBackgroundColour(Theme::BG);
    this->pagePlay->SetBackgroundStyle(wxBG_STYLE_PAINT);
    this->pagePlay->Bind(wxEVT_PAINT, &cMain::OnPlayPagePaint, this);

    this->btn_Play = new FlatButton(this->pagePlay, ID_PLAY, L"PLAY",
        wxDefaultPosition, wxSize(300, 84));
    this->btn_Play->SetFont(Theme::Font(22));

    this->lbl_Status = new wxStaticText(this->pagePlay, wxID_ANY, L"Ready",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    this->lbl_Status->SetForegroundColour(Theme::FG_DIM);
    this->lbl_Status->SetBackgroundColour(Theme::BG);
    this->lbl_Status->SetFont(Theme::Font(8));

    this->pagePlay->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) {
        wxSize sz = this->pagePlay->GetClientSize();
        int w = sz.GetWidth(), h = sz.GetHeight();
        wxSize ps = this->btn_Play->GetSize();
        int px = (w - ps.GetWidth()) / 2;
        int py = h - ps.GetHeight() - 90;
        this->btn_Play->SetPosition(wxPoint(px, py));
        this->lbl_Status->SetSize(w, -1);
        this->lbl_Status->SetPosition(wxPoint(0, py + ps.GetHeight() + 20));
        this->pagePlay->Refresh();
        e.Skip();
    });

    // =================== SETTINGS PAGE ===================
    this->pageSettings = new wxPanel(this, wxID_ANY);
    this->pageSettings->SetBackgroundColour(Theme::BG);

    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

    // DLL source card
    wxPanel* card1 = new wxPanel(this->pageSettings, wxID_ANY);
    card1->SetBackgroundColour(Theme::CARD);
    wxBoxSizer* c1 = new wxBoxSizer(wxVERTICAL);
    c1->Add(MakeLabel(card1, L"DLL SOURCE", Theme::CARD), 0, wxALL, 12);

    this->radio_Local = new wxRadioButton(card1, ID_RADIO_LOCAL, L"Local DLL file",
        wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    this->radio_Github = new wxRadioButton(card1, ID_RADIO_GITHUB, L"GitHub (latest release)");
    StyleRadio(this->radio_Local);
    StyleRadio(this->radio_Github);
    c1->Add(this->radio_Local, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    c1->Add(this->radio_Github, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

    wxBoxSizer* localRow = new wxBoxSizer(wxHORIZONTAL);
    this->txt_LocalPath = new wxTextCtrl(card1, wxID_ANY,
        (Globals::LOCAL_DLL_PATH.empty() ? Globals::NO_DLL_PATH_SELECTED_MSG : Globals::LOCAL_DLL_PATH),
        wxDefaultPosition, wxSize(-1, 30));
    StyleInput(this->txt_LocalPath);
    this->btn_Browse = new FlatButton(card1, ID_BROWSE, L"BROWSE", wxDefaultPosition, wxSize(90, 34));
    this->btn_Browse->SetFont(Theme::Font(9));
    localRow->Add(this->txt_LocalPath, 1, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 8);
    localRow->Add(this->btn_Browse, 0);
    c1->Add(localRow, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    card1->SetSizer(c1);
    root->Add(card1, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 14);

    // Minecraft path card
    wxPanel* card2 = new wxPanel(this->pageSettings, wxID_ANY);
    card2->SetBackgroundColour(Theme::CARD);
    wxBoxSizer* c2 = new wxBoxSizer(wxVERTICAL);
    c2->Add(MakeLabel(card2, L"MINECRAFT PATH", Theme::CARD), 0, wxALL, 12);
    this->txt_McPath = new wxTextCtrl(card2, wxID_ANY, Globals::MC_PATH,
        wxDefaultPosition, wxSize(-1, 30));
    StyleInput(this->txt_McPath);
    c2->Add(this->txt_McPath, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
    c2->Add(MakeLabel(card2, L"URI protocol, or path to the game .appx / .exe", Theme::CARD, true),
        0, wxALL, 12);
    card2->SetSizer(c2);
    root->Add(card2, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 14);

    root->AddStretchSpacer(1);

    this->btn_Save = new FlatButton(this->pageSettings, ID_SAVE, L"SAVE SETTINGS",
        wxDefaultPosition, wxSize(-1, 52));
    this->btn_Save->SetFont(Theme::Font(11));
    root->Add(this->btn_Save, 0, wxEXPAND | wxALL, 14);

    this->pageSettings->SetSizer(root);

    // Initialize radio + enabled state
    if (Globals::DLL_SOURCE == Globals::SOURCE_LOCAL) this->radio_Local->SetValue(true);
    else this->radio_Github->SetValue(true);
    this->refreshEnabledState();

    this->Bind(EVT_PLAY_STATUS, &cMain::OnPlayStatus, this);
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { this->layoutTabs(); e.Skip(); });

    this->SetMinSize(wxSize(400, 560));
    this->ShowPage(false);
    this->layoutTabs();
}

cMain::~cMain() {
    if (this->worker.joinable()) this->worker.join();
}

void cMain::layoutTabs() {
    wxSize sz = this->GetClientSize();
    int w = sz.GetWidth(), h = sz.GetHeight();
    int halfW = w / 2;
    this->tab_Play->SetSize(0, 0, halfW, TAB_H);
    this->tab_Settings->SetSize(halfW, 0, w - halfW, TAB_H);
    this->pagePlay->SetSize(0, TAB_H, w, h - TAB_H);
    this->pageSettings->SetSize(0, TAB_H, w, h - TAB_H);
}

void cMain::ShowPage(bool settings) {
    this->tab_Play->SetSelected(!settings);
    this->tab_Settings->SetSelected(settings);
    this->pagePlay->Show(!settings);
    this->pageSettings->Show(settings);
    this->layoutTabs();
    if (settings) this->pageSettings->Layout();
}

void cMain::OnTabPlay(wxCommandEvent&)     { this->ShowPage(false); }
void cMain::OnTabSettings(wxCommandEvent&) { this->ShowPage(true); }

void cMain::OnPlayPagePaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this->pagePlay);
    dc.SetBackground(wxBrush(Theme::BG));
    dc.Clear();

    wxSize sz = this->pagePlay->GetClientSize();
    int w = sz.GetWidth();

    // Grey emblem block, centered in the upper area.
    const int box = 140;
    int bx = (w - box) / 2;
    int by = 60;

    // outline + face
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(Theme::OUTLINE));
    dc.DrawRectangle(bx - 4, by - 4, box + 8, box + 8);
    dc.SetBrush(wxBrush(Theme::CARD));
    dc.DrawRectangle(bx, by, box, box);
    // inner bevel
    dc.SetBrush(wxBrush(wxColour(58, 58, 58)));
    dc.DrawRectangle(bx, by, box, 5);
    dc.DrawRectangle(bx, by, 5, box);
    dc.SetBrush(wxBrush(wxColour(20, 20, 20)));
    dc.DrawRectangle(bx, by + box - 5, box, 5);
    dc.DrawRectangle(bx + box - 5, by, 5, box);
    // corner pixels
    dc.SetBrush(wxBrush(Theme::STONE));
    dc.DrawRectangle(bx - 2, by - 2, 9, 9);
    dc.DrawRectangle(bx + box - 7, by - 2, 9, 9);
    dc.DrawRectangle(bx - 2, by + box - 7, 9, 9);
    dc.DrawRectangle(bx + box - 7, by + box - 7, 9, 9);

    // emblem text
    dc.SetFont(Theme::Font(12));
    wxString t1 = L"CONTROLLIN";
    wxCoord tw, th;
    dc.GetTextExtent(t1, &tw, &th);
    dc.SetTextForeground(Theme::SHADOW);
    dc.DrawText(t1, bx + (box - tw) / 2 + 2, by + box / 2 - th - 2 + 2);
    dc.SetTextForeground(Theme::FG);
    dc.DrawText(t1, bx + (box - tw) / 2, by + box / 2 - th - 2);

    dc.SetFont(Theme::Font(8));
    wxString t2 = L"bedrock injector";
    dc.GetTextExtent(t2, &tw, &th);
    dc.SetTextForeground(Theme::FG_DIM);
    dc.DrawText(t2, bx + (box - tw) / 2, by + box / 2 + 4);
}

void cMain::postStatus(const std::wstring& msg) {
    wxThreadEvent* evt = new wxThreadEvent(EVT_PLAY_STATUS);
    evt->SetPayload<wxString>(wxString(msg));
    wxQueueEvent(this, evt);
}

void cMain::OnPlayStatus(wxThreadEvent& evt) {
    wxString msg = evt.GetPayload<wxString>();
    if (this->lbl_Status) {
        this->lbl_Status->SetLabel(msg);
        this->pagePlay->Layout();
        this->pagePlay->Refresh();
    }
}

void cMain::OnPlayButton(wxCommandEvent&) {
    if (this->busy.load()) return;
    if (this->worker.joinable()) this->worker.join();

    this->busy.store(true);
    this->btn_Play->Enable(false);
    this->btn_Play->SetCaption(L"...");

    this->worker = std::thread(&cMain::PlayWorker, this);
}

void cMain::PlayWorker() {
    auto finish = [this](const std::wstring& msg) {
        this->postStatus(msg);
        this->busy.store(false);
        this->CallAfter([this] {
            this->btn_Play->Enable(true);
            this->btn_Play->SetCaption(L"PLAY");
        });
    };

    DWORD procId = GetProcId("Minecraft.Windows.exe");

    if (procId == 0) {
        this->postStatus(L"Launching Minecraft...");
        if (!LaunchMinecraft(Globals::MC_PATH)) {
            finish(L"Failed to launch Minecraft");
            return;
        }
        this->postStatus(L"Waiting for Minecraft...");
        procId = WaitForMinecraft(20000);
        if (procId == 0) {
            finish(L"Minecraft did not start in time");
            return;
        }
    }

    std::wstring dllPath;
    if (Globals::DLL_SOURCE == Globals::SOURCE_GITHUB) {
        this->postStatus(L"Downloading latest DLL...");
        std::wstring status;
        dllPath = DownloadLatestGithubDll(status);
        if (dllPath.empty()) {
            finish(L"Download failed: " + status);
            return;
        }
    } else {
        dllPath = Globals::LOCAL_DLL_PATH;
        if (dllPath.empty()) {
            finish(L"No local DLL selected (open Settings)");
            return;
        }
    }

    std::ifstream test(dllPath.c_str());
    if (!test) {
        finish(L"DLL file not found");
        return;
    }
    test.close();

    this->postStatus(L"Injecting...");
    SetAccessControl(dllPath, Globals::ALL_APP_PACKAGES_SID.c_str());
    performInjection(procId, dllPath.c_str());

    finish(L"Injected! Enjoy :)");
}

void cMain::refreshEnabledState() {
    bool local = this->radio_Local->GetValue();
    this->txt_LocalPath->Enable(local);
    this->btn_Browse->Enable(local);
}

void cMain::OnSourceChanged(wxCommandEvent&) { this->refreshEnabledState(); }

void cMain::OnBrowse(wxCommandEvent&) {
    wxFileDialog openDialog(this, L"Select a DLL or EXE file", Globals::WORKING_DIR, L"",
        L"Injectable files (*.dll;*.exe)|*.dll;*.exe|All files (*.*)|*.*",
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_OK) {
        this->txt_LocalPath->SetValue(openDialog.GetPath());
    }
}

void cMain::OnSave(wxCommandEvent&) {
    Globals::DLL_SOURCE = this->radio_Local->GetValue() ? Globals::SOURCE_LOCAL : Globals::SOURCE_GITHUB;

    wxString localPath = this->txt_LocalPath->GetValue();
    if (localPath == Globals::NO_DLL_PATH_SELECTED_MSG) {
        Globals::LOCAL_DLL_PATH = std::wstring{};
    } else {
        Globals::LOCAL_DLL_PATH = localPath.ToStdWstring();
    }

    wxString mcPath = this->txt_McPath->GetValue();
    if (!mcPath.IsEmpty()) {
        Globals::MC_PATH = mcPath.ToStdWstring();
    }

    this->cfg.updateConfigFile();

    this->lbl_Status->SetLabel(L"Settings saved");
    this->ShowPage(false);
}