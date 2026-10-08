#pragma once

#include <wx/wx.h>

// Owner-drawn Minecraft-style "stone" button with a hard pixel bevel.
// Emits a normal wxEVT_BUTTON with its id, so EVT_BUTTON tables work as usual.
class FlatButton : public wxWindow {
public:
    enum Style {
        STYLE_STONE,  // grey stone button (default)
        STYLE_TAB     // flat tab header with an underline when selected
    };

    FlatButton(wxWindow* parent, wxWindowID id, const wxString& label,
               const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);

    void SetColors(const wxColour& base, const wxColour& hover, const wxColour& text);
    void SetCaption(const wxString& s) { caption = s; Refresh(); }
    void SetButtonStyle(Style s) { style = s; Refresh(); }
    void SetSelected(bool on) { selected = on; Refresh(); }   // for STYLE_TAB
    void SetTabAccent(const wxColour& c) { tabAccent = c; Refresh(); } // for STYLE_TAB: custom text colour

    bool Enable(bool enable = true) override;

private:
    wxString caption;
    wxColour colBase, colHover, colText;
    Style style = STYLE_STONE;
    bool selected = false;
    wxColour tabAccent;   // invalid = default tab colours
    bool hovering = false;
    bool pressed = false;

    void OnPaint(wxPaintEvent&);
    void OnEnter(wxMouseEvent&);
    void OnLeave(wxMouseEvent&);
    void OnDown(wxMouseEvent&);
    void OnUp(wxMouseEvent&);

    void DrawTextWithShadow(wxDC& dc, const wxString& text, int x, int y,
                            const wxColour& fg);
};
