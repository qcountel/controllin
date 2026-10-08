#include "pch.h"
#include "cMain.h"
#include "launcher.h"
#include "inject.h"
#include "theme.h"

enum {
    ID_TAB_PLAY = 101,
    ID_TAB_PLUS = 102,
    ID_PLAY = 103,
    ID_PLUS_OK = 104,
};

static const int TAB_H = 42;

wxDEFINE_EVENT(EVT_PLAY_STATUS, wxThreadEvent);

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(ID_TAB_PLUS, cMain::OnPlus)
EVT_BUTTON(ID_PLAY, cMain::OnPlayButton)
wxEND_EVENT_TABLE();

// ---------------------------------------------------------------------------
// "Controllin +" placeholder dialog (subscription is not available yet)
// ---------------------------------------------------------------------------
class PlusDialog : public wxDialog {
public:
    explicit PlusDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, L"Controllin +", wxDefaultPosition, wxDefaultSize,
                   wxCAPTION | wxCLOSE_BOX) {
        Theme::ApplyDarkTitleBar((HWND)this->GetHandle());
        this->SetBackgroundColour(Theme::BG);

        wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);

        wxStaticText* title = new wxStaticText(this, wxID_ANY, L"CONTROLLIN +");
        title->SetForegroundColour(Theme::GOLD);
        title->SetBackgroundColour(Theme::BG);
        title->SetFont(Theme::Font(16));
        root->Add(title, 0, wxALIGN_CENTRE_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 24);

        wxStaticText* text = new wxStaticText(this, wxID_ANY, L"Subscription is coming soon!",
            wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
        text->SetForegroundColour(Theme::FG);
        text->SetBackgroundColour(Theme::BG);
        text->SetFont(Theme::Font(10));
        root->Add(text, 0, wxALIGN_CENTRE_HORIZONTAL | wxLEFT | wxRIGHT | wxTOP, 18);

        FlatButton* ok = new FlatButton(this, ID_PLUS_OK, L"OK", wxDefaultPosition, wxSize(140, 44));
        ok->SetFont(Theme::Font(11));
        ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { this->EndModal(wxID_OK); });
        root->Add(ok, 0, wxALIGN_CENTRE_HORIZONTAL | wxALL, 24);

        this->SetSizerAndFit(root);
        this->CentreOnParent();
    }
};

cMain::cMain()
    : wxFrame(nullptr, wxID_ANY, L"Controllin Injector", wxDefaultPosition, wxSize(400, 600),
              wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN) {

    Theme::EnsurePixelFont();

    // Window / taskbar icon: all sizes from the embedded .ico, fall back to the XPM.
    wxIconBundle icons(L"appicon", nullptr);
    if (icons.IsEmpty()) icons.AddIcon(wxIcon(icon_xpm));
    this->SetIcons(icons);

    Theme::ApplyDarkTitleBar((HWND)this->GetHandle());
    this->SetBackgroundColour(Theme::BG);

    // ---- Header ----
    this->tab_Play = new FlatButton(this, ID_TAB_PLAY, L"PLAY");
    this->tab_Play->SetButtonStyle(FlatButton::STYLE_TAB);
    this->tab_Play->SetFont(Theme::Font(10));
    this->tab_Play->SetSelected(true);

    this->tab_Plus = new FlatButton(this, ID_TAB_PLUS, L"CONTROLLIN +");
    this->tab_Plus->SetButtonStyle(FlatButton::STYLE_TAB);
    this->tab_Plus->SetFont(Theme::Font(10));
    this->tab_Plus->SetTabAccent(Theme::GOLD);

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

    this->Bind(EVT_PLAY_STATUS, &cMain::OnPlayStatus, this);
    this->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { this->layoutTabs(); e.Skip(); });

    this->SetMinSize(wxSize(400, 560));
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
    this->tab_Plus->SetSize(halfW, 0, w - halfW, TAB_H);
    this->pagePlay->SetSize(0, TAB_H, w, h - TAB_H);
}

void cMain::OnPlus(wxCommandEvent&) {
    PlusDialog dlg(this);
    dlg.ShowModal();
}

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

    // The DLL is always the latest GitHub release.
    this->postStatus(L"Downloading latest DLL...");
    std::wstring status;
    std::wstring dllPath = DownloadLatestGithubDll(status);
    if (dllPath.empty()) {
        finish(L"Download failed: " + status);
        return;
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
