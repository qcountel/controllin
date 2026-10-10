#include "pch.h"
#include "cMain.h"
#include "launcher.h"
#include "inject.h"
#include "theme.h"
#include "hwid.h"

enum {
    ID_TAB_PLAY = 101,
    ID_TAB_PLUS = 102,
    ID_PLAY = 103,
    ID_PLUS_OK = 104,
    ID_VERSION = 105,
};

static const int TAB_H = 42;

wxDEFINE_EVENT(EVT_PLAY_STATUS, wxThreadEvent);

wxBEGIN_EVENT_TABLE(cMain, wxFrame)
EVT_BUTTON(ID_TAB_PLUS, cMain::OnPlus)
EVT_BUTTON(ID_PLAY, cMain::OnPlayButton)
EVT_BUTTON(ID_VERSION, cMain::OnVersionButton)
wxEND_EVENT_TABLE();

// ---------------------------------------------------------------------------
// "Controllin +" dialog: price, where to buy, and the HWID needed for the key
// ---------------------------------------------------------------------------
class PlusDialog : public wxDialog {
    static wxStaticText* Text(wxWindow* parent, const wxString& s, const wxColour& fg, int pt) {
        wxStaticText* t = new wxStaticText(parent, wxID_ANY, s, wxDefaultPosition, wxDefaultSize,
                                           wxALIGN_CENTRE_HORIZONTAL);
        t->SetForegroundColour(fg);
        t->SetBackgroundColour(Theme::BG);
        t->SetFont(Theme::Font(pt));
        return t;
    }

public:
    explicit PlusDialog(wxWindow* parent)
        : wxDialog(parent, wxID_ANY, L"Controllin +", wxDefaultPosition, wxDefaultSize,
                   wxCAPTION | wxCLOSE_BOX) {
        Theme::ApplyDarkTitleBar((HWND)this->GetHandle());
        this->SetBackgroundColour(Theme::BG);

        const wxString hwid = GetHWID();
        const int flags = wxALIGN_CENTRE_HORIZONTAL | wxLEFT | wxRIGHT;

        wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
        root->Add(Text(this, L"CONTROLLIN +", Theme::GOLD, 16), 0, flags | wxTOP, 24);

        // ---- Price ----
        root->Add(Text(this, wxString(L"Подписка: ") + Globals::PLUS_PRICE + L" навсегда", Theme::FG, 11),
                  0, flags | wxTOP, 18);
        root->Add(Text(this, L"Купить можно в Telegram,\nнаписав владельцу", Theme::FG_DIM, 9),
                  0, flags | wxTOP, 10);
        root->Add(Text(this, Globals::TELEGRAM_USER, Theme::GOLD, 10), 0, flags | wxTOP, 6);

        FlatButton* tg = new FlatButton(this, wxID_ANY, L"НАПИСАТЬ В TELEGRAM", wxDefaultPosition, wxSize(260, 40));
        tg->SetFont(Theme::Font(10));
        tg->Bind(wxEVT_BUTTON, [](wxCommandEvent&) { wxLaunchDefaultBrowser(Globals::TELEGRAM_URL); });
        root->Add(tg, 0, flags | wxTOP, 12);

        // ---- HWID ----
        root->Add(Text(this, L"Ваш HWID:", Theme::FG_DIM, 9), 0, flags | wxTOP, 22);
        root->Add(Text(this, hwid.empty() ? wxString(L"не удалось получить") : hwid, Theme::FG, 10),
                  0, flags | wxTOP, 6);
        root->Add(Text(this, L"HWID нужен для создания\nключа подписки", Theme::FG_DIM, 8), 0, flags | wxTOP, 6);

        FlatButton* copy = new FlatButton(this, wxID_ANY, L"СКОПИРОВАТЬ HWID", wxDefaultPosition, wxSize(260, 40));
        copy->SetFont(Theme::Font(10));
        copy->Enable(!hwid.empty());
        copy->Bind(wxEVT_BUTTON, [copy, hwid](wxCommandEvent&) {
            if (wxTheClipboard->Open()) {
                wxTheClipboard->SetData(new wxTextDataObject(hwid));
                wxTheClipboard->Flush();   // keep it after the injector is closed
                wxTheClipboard->Close();
                copy->SetCaption(L"СКОПИРОВАНО!");
            } else {
                copy->SetCaption(L"БУФЕР ЗАНЯТ");
            }
        });
        root->Add(copy, 0, flags | wxTOP, 12);

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
    LoadSelectedVersion();

    // Window / taskbar icon: all sizes from the embedded .ico, fall back to the XPM.
    wxIconBundle icons(L"CONTROLLIN_ICON", nullptr);
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

    this->btn_Version = new FlatButton(this->pagePlay, ID_VERSION, L"",
        wxDefaultPosition, wxSize(300, 40));
    this->btn_Version->SetFont(Theme::Font(10));
    this->btn_Version->SetToolTip(L"Choose the game version");
    this->refreshVersionButton();

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
        wxSize vs = this->btn_Version->GetSize();
        this->btn_Version->SetPosition(wxPoint((w - vs.GetWidth()) / 2, py - vs.GetHeight() - 14));
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

void cMain::refreshVersionButton() {
    this->btn_Version->SetCaption(wxString(L"VERSION: ") + Globals::VERSIONS[Globals::SELECTED_VERSION].name);
}

void cMain::OnVersionButton(wxCommandEvent&) {
    if (this->busy.load()) return;
    wxMenu menu;
    for (int i = 0; i < Globals::VERSION_COUNT; ++i)
        menu.AppendRadioItem(wxID_HIGHEST + 1 + i, Globals::VERSIONS[i].name)->Check(i == Globals::SELECTED_VERSION);
    menu.Bind(wxEVT_MENU, [this](wxCommandEvent& e) {
        int i = e.GetId() - wxID_HIGHEST - 1;
        if (i < 0 || i >= Globals::VERSION_COUNT || i == Globals::SELECTED_VERSION) return;
        Globals::SELECTED_VERSION = i;
        SaveSelectedVersion();
        this->refreshVersionButton();
        this->postStatus(std::wstring(L"Selected ") + Globals::VERSIONS[i].name);
    });
    wxPoint pos = this->btn_Version->GetPosition();
    this->pagePlay->PopupMenu(&menu, pos.x, pos.y + this->btn_Version->GetSize().GetHeight());
}

void cMain::OnPlayButton(wxCommandEvent&) {
    if (this->busy.load()) return;
    if (this->worker.joinable()) this->worker.join();

    this->busy.store(true);
    this->btn_Play->Enable(false);
    this->btn_Version->Enable(false);
    this->btn_Play->SetCaption(L"...");

    this->worker = std::thread(&cMain::PlayWorker, this);
}

void cMain::PlayWorker() {
    auto finish = [this](const std::wstring& msg) {
        this->postStatus(msg);
        this->busy.store(false);
        this->CallAfter([this] {
            this->btn_Play->Enable(true);
            this->btn_Version->Enable(true);
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

    // The DLL is the newest GitHub release tagged with the selected version ("v2.0.5 1.16").
    const Globals::GameVersion& ver = Globals::VERSIONS[Globals::SELECTED_VERSION];
    this->postStatus(std::wstring(L"Downloading DLL for ") + ver.name + L"...");
    std::wstring status, release;
    std::wstring dllPath = DownloadLatestGithubDll(ver.tag, release, status);
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

    finish(L"Injected " + release + L"! Enjoy :)");
}
