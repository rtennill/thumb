//  Copyright (C) 2005 Robert Kooima
//
//  THUMB is free software; you can redistribute it and/or modify it under
//  the terms of  the GNU General Public License as  published by the Free
//  Software  Foundation;  either version 2  of the  License,  or (at your
//  option) any later version.
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.

#include <algorithm>
#include <iostream>
#include <SDL_keyboard.h>

#include "gui-gui.hpp"
#include "app-data.hpp"
#include "app-conf.hpp"
#include "app-lang.hpp"
#include "app-user.hpp"
#include "app-host.hpp"
#include "util.hpp"

//-----------------------------------------------------------------------------
// Basic widget.

gui::widget::widget() : area(0, 0, 0, 0)
{
    is_enabled = false;
    is_pressed = false;
}

gui::widget::~widget()
{
    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        delete (*i);

    child.clear();
}

gui::widget *gui::widget::click(int, int, int, bool d)
{
    is_pressed = d;
    return 0;
}

void gui::widget::value(std::string)
{
}

std::string gui::widget::value() const
{
    return "";
}

void gui::widget::laydn(int x, int y, int w, int h)
{
    area.x = x;
    area.y = y;
    area.w = w;
    area.h = h;
}

void gui::widget::back_color(const widget *focus) const
{
    // Set the background color.

    if      (is_pressed)    glColor4f(0.5f, 0.5f, 0.5f, 0.7f);
    else if (this == focus) glColor4f(0.3f, 0.3f, 0.3f, 0.7f);
    else if (is_enabled)    glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
    else                    glColor4f(0.1f, 0.1f, 0.1f, 0.7f);
}

void gui::widget::fore_color() const
{
    // Set the foreground color.

    if (is_enabled)
        glColor4f(0.0f, 0.0f, 0.0f, 0.6f);
    else
        glColor4f(0.0f, 0.0f, 0.0f, 0.2f);
}

//-----------------------------------------------------------------------------
// Leaf widget.

gui::widget *gui::leaf::enter(int x, int y)
{
    // If the point is within this widget, return this.

    return (is_enabled && area.test(x, y)) ? this : 0;
}

void gui::leaf::draw(const widget *focus, const widget *input) const
{
    back_color(focus);

    // Draw the background.

    glBegin(GL_QUADS);
    {
        glVertex2i(area.L(), area.B());
        glVertex2i(area.R(), area.B());
        glVertex2i(area.R(), area.T());
        glVertex2i(area.L(), area.T());
    }
    glEnd();
}

//-----------------------------------------------------------------------------
// Tree widget.

bool gui::tree::exp_w() const
{
    bool b = false;

    // A group is horizontally expanding if any child is.

    for (widget_v::const_iterator i = child.begin(); i != child.end(); ++i)
        b |= (*i)->exp_w();

    return b;
}

bool gui::tree::exp_h() const
{
    bool b = false;

    // A group is vertically expanding if any child is.

    for (widget_v::const_iterator i = child.begin(); i != child.end(); ++i)
        b |= (*i)->exp_h();

    return b;
}

gui::widget *gui::tree::enter(int x, int y)
{
    widget *w;

    // Search all child nodes for the widget containing the given point.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        if ((w = (*i)->enter(x, y)))
            return w;

    return 0;
}

void gui::tree::show()
{
    // Show all child nodes.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        (*i)->show();
}

void gui::tree::hide()
{
    // Hide all child nodes.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        (*i)->hide();
}

void gui::tree::draw(const widget *focus, const widget *input) const
{
    // Draw all child nodes.

    for (widget_v::const_iterator i = child.begin(); i != child.end(); ++i)
        (*i)->draw(focus, input);
}

gui::tree::~tree()
{
    // Delete all child nodes.

    for (widget_v::const_iterator i = child.begin(); i != child.end(); ++i)
        delete (*i);

    child.clear();
}

//-----------------------------------------------------------------------------
// Basic string widget.

app::font *gui::string::sans_font = 0;
app::font *gui::string::mono_font = 0;

int gui::string::count = 0;

void gui::string::init_font()
{
    if (count == 0)
    {
        sans_font = new app::font(::conf->get_s("sans_font"),
                                  ::conf->get_i("sans_size"));
        mono_font = new app::font(::conf->get_s("mono_font"),
                                  ::conf->get_i("mono_size"));
    }
    count++;
}

void gui::string::free_font()
{
    count--;

    if (count == 0)
    {
        delete sans_font;
        delete mono_font;
    }
}

//-----------------------------------------------------------------------------

gui::string::string(std::string s, int f, int j, GLubyte r,
                                                 GLubyte g,
                                                 GLubyte b) :
    text(0), just(j), str(lang->get(s))
{
    // Initialize the font.

    init_font();

    switch (f)
    {
    case sans: font = sans_font; break;
    case mono: font = mono_font; break;
    }

    color[0] = r;
    color[1] = g;
    color[2] = b;

    // Render the string texture.

    init_text();

    // Pad the string.

    area.w = text->w() + font->size() * 2;
    area.h = text->h() + font->size();

    just_text();
}

gui::string::~string()
{
    if (text) delete text;

    free_font();
}

void gui::string::init_text()
{
    if (text) delete text;

    // Render the new string texture.

    text = font->render(str);
}

void gui::string::draw_text() const
{
    // Draw the string.

    glPushAttrib(GL_ENABLE_BIT);
    {
        glEnable(GL_TEXTURE_2D);

        glColor3ubv(color);
        text->draw();
    }
    glPopAttrib();
}

void gui::string::just_text()
{
    // Set the justification of the new string.

    int jx = (area.w - text->w()) / 2;
    int jy = (area.h - text->h()) / 2;

    if (just > 0) jx = area.w - text->w() - 4;
    if (just < 0) jx = 4;

    text->move(area.x + jx, area.y + jy);
}

void gui::string::value(std::string s)
{
    str = lang->get(s);
    init_text();
    just_text();
}

std::string gui::string::value() const
{
    return str;
}

void gui::string::laydn(int x, int y, int w, int h)
{
    leaf::laydn(x, y, w, h);
    just_text();
}

void gui::string::draw(const widget *focus, const widget *input) const
{
    leaf::draw(focus, input);
    draw_text();
}

//-----------------------------------------------------------------------------
// Pushbutton widget.

gui::button::button(std::string text, int font, int just, int border) :
    string(text, font, just), border(border)
{
    is_enabled = true;
}

void gui::button::draw(const widget *focus, const widget *input) const
{
    // Draw the background.

    leaf::draw(focus, input);

    // Draw the button shape.

    glBegin(GL_QUADS);
    {
        fore_color();

        glVertex2i(area.L() + border, area.B() + border);
        glVertex2i(area.R() - border, area.B() + border);
        glVertex2i(area.R() - border, area.T() - border);
        glVertex2i(area.L() + border, area.T() - border);
    }
    glEnd();

    // Draw the text.

    draw_text();
}

gui::widget *gui::button::click(int x, int y, int m, bool d)
{
    widget *input = string::click(x, y, m, d);

    // Activate the button on mouse up.

    if (d == false) apply();
    return input;
}

//-----------------------------------------------------------------------------
// String input widget.

gui::input::input(std::string t, int f, int j) : string(t, f, j)
{
    is_enabled = true;
    is_changed = false;
}

void gui::input::hide()
{
    if (is_changed) apply();
}

//-----------------------------------------------------------------------------
// Bitmap widget.

gui::bitmap::bitmap() :
    input("0123456789ABCDEFGHIJKLMNOPQRSTUV", mono), bits(0)
{
}

void gui::bitmap::draw(const widget *focus, const widget *input) const
{
    leaf::draw(focus, input);

    // Draw the string.

    glPushAttrib(GL_ENABLE_BIT);
    {
        glEnable(GL_TEXTURE_2D);

        for (int i = 0, b = 1; i < text->n(); ++i, b <<= 1)
        {
            if (bits & b)
                glColor3ubv(color);
            else
                glColor3f(0.0f, 0.0f, 0.0f);
            
            text->draw(i);
        }
    }
    glPopAttrib();
}

gui::widget *gui::bitmap::click(int x, int y, int m, bool d)
{
    if (d == false)
    {
        int i;

        if ((i = text->find(x, y)) >= 0)
        {
            // If shift, set all bits to the toggle of bit i.

            if (m & KMOD_SHIFT)
            {
                if ((bits & (1 << i)))
                    bits = 0x00000000;
                else
                    bits = 0xFFFFFFFF;
            }

            // If control, toggle all bits.

            else if (m & KMOD_CTRL)
                for (int j = 0; j < text->n(); ++j)
                    bits = bits ^ (1 << j);

            // Otherwise toggle bit i.

            else bits = bits ^ (1 << i);

            is_changed = true;

            return this;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Editable text widget.

std::string gui::editor::clip;

gui::editor::editor(std::string t) :
    input(t, mono, -1), si(0), sc(0)
{
}

void gui::editor::update()
{
    input::init_text();
    input::just_text();

    // Make sure the cursor position is sane.

    si = std::min(si, text->n());
    sc = 0;
}

void gui::editor::grow_select(int sd)
{
    int sn = int(str.length());
    int sj = si;

    // Extend the selection.

    if (sd > 0)
    {
        sc += sd;
        sc  = std::min(sc, sn - si);
    }
    else
    {
        si += sd;
        si  = std::max(si, 0);
        sc += (sj - si);
    }
}

void gui::editor::move_select(int sd)
{
    int sn = int(str.length());

    // Move the cursor.

    if (sc)
    {
        if (sd > 0)
            si += sc + sd - 1;
        else
            si +=      sd + 1;
    }
    else
        si += sd;

    sc = 0;

    si = std::max(si, 0);
    si = std::min(si, sn);
}

void gui::editor::draw(const widget *focus, const widget *input) const
{
    leaf::draw(focus, input);

    glBegin(GL_QUADS);
    {
        // Draw the editor shape.

        fore_color();

        glVertex2i(area.L() + 2, area.B() + 2);
        glVertex2i(area.R() - 2, area.B() + 2);
        glVertex2i(area.R() - 2, area.T() - 2);
        glVertex2i(area.L() + 2, area.T() - 2);

        // Draw the selection / cursor.

        if (this == input)
        {
            int L = text->curs(0) - 1;
            int R = text->curs(0) + 1;

            if (sc)
            {
                L = text->curs(si     );
                R = text->curs(si + sc);
            }
            else if (si)
            {
                L = text->curs(si) - 1;
                R = text->curs(si) + 1;
            }

            glColor4ub(0xFF, 0xC0, 0x40, 0x80);

            glVertex2i(L, area.B() + 3);
            glVertex2i(R, area.B() + 3);
            glVertex2i(R, area.T() - 3);
            glVertex2i(L, area.T() - 3);
        }
    }
    glEnd();

    // Draw the text.

    draw_text();
}

gui::widget *gui::editor::click(int x, int y, int m, bool d)
{
    int sp = text->find(x, y);

    leaf::click(x, y, m, d);

    if (sp >= 0)
    {
        if (d)
        {
            si = sp;
            sc = 0;
        }
    }
    else
    {
        if (d)
        {
            si = int(str.length());
            sc = 0;
        }
    }
    return this;
}

void gui::editor::point(int x, int y)
{
    int sp = text->find(x, y);

    if (sp >= 0)
    {
        if (sp > si)
            sc = sp - si;
        else
        {
            sc = si + sc - sp;
            si = sp;
        }
    }
    else sc = int(str.length()) - si;
}

void gui::editor::keybd(int c, int k, int m)
{
    int n = int(str.length());
    int s = (k & KMOD_SHIFT);

    switch (k)
    {
    // Handle cursor and selection keys.

    case SDLK_LEFT:  if (s) grow_select(-1); else move_select(-1); return;
    case SDLK_RIGHT: if (s) grow_select(+1); else move_select(+1); return;
    case SDLK_HOME:  if (s) grow_select(-n); else move_select(-n); return;
    case SDLK_END:   if (s) grow_select(+n); else move_select(+n); return;

    case SDLK_RETURN:
        update();
        return;

   // Handle deletion to the left.

    case SDLK_BACKSPACE:
        if (sc)
            str.erase(si, sc);
        else if (si > 0)
            str.erase(--si, 1);

        sc = 0;
        update();
        is_changed = true;
        return;

    // Handle deletion to the right.

    case SDLK_DELETE:
        if (sc)
            str.erase(si, sc);
        else
            str.erase(si, 1);

        sc = 0;
        update();
        is_changed = true;
        return;
    }

    // Handle text copy.

    if (c == 3 && sc > 0)
        clip = str.substr(si, sc);

    // Handle text paste.

    if (c == 22 && clip.length() > 0)
    {
        str.replace(si, sc, clip);
        si = si + int(clip.length());
        sc = 0;
        update();
        is_changed = true;
    }

    // Handle text insertion.

    if (32 <= c && c < 127)
    {
        str.replace(si, sc, std::string(1, c));
        si = si + 1;
        sc = 0;
        update();
        is_changed = true;
    }
}

//-----------------------------------------------------------------------------
// Vertically scrolling panel.

void gui::scroll::layup()
{
    // Find the widest child width.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(),  get_w());
        area.h =         ((*i)->get_h() + get_h());
    }

    // Include the scrollbar width.  Height is arbitrary.

    area.w += scroll_w;

    child_h = area.h;
    area.h  = 0;
}

void gui::scroll::laydn(int x, int y, int w, int h)
{
    int c = 0, excess = std::max(h - child_h, 0);

    widget::laydn(x, y, w, h);

    // Count the number of expanding children.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        if ((*i)->exp_h())
            c++;

    // Give children requested vertical space.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        int dy = (*i)->get_h() + ((*i)->exp_h() ? (excess / c) : 0);

        (*i)->laydn(x, y + h - dy, w - scroll_w, dy);

        y -= dy;
    }
}

gui::widget *gui::scroll::click(int x, int y, int m, bool d)
{
    widget *input = widget::click(x, y, m, d);

    // Focus only lands here when the pointer is in the scrollbar.

    point(x, y);
    return input;
}

void gui::scroll::point(int x, int y)
{
    int m;

    // We know the pointer is in the scrollbar, so scroll.

    if ((m = std::max(child_h - area.h, 0)) > 0)
    {
        int thumb_h = (area.h - 4) * (area.h - 4) / child_h;

        child_d = m - m * (y - (area.y + thumb_h / 2 + 2)) /
                          (    (area.h - thumb_h     - 4));

        if (child_d > m) child_d = m;
        if (child_d < 0) child_d = 0;
    }
    else child_d = 0;
}

gui::widget *gui::scroll::enter(int x, int y)
{
    // Check for a hit on the scrollbar, or search the child list.

    if (is_enabled)
    {
        rect C(area.x,                     area.y, area.w - scroll_w, area.h);
        rect S(area.x + area.w - scroll_w, area.y,          scroll_w, area.h);

        if (C.test(x, y)) return tree::enter(x, y - child_d);
        if (S.test(x, y)) return this;
    }
    return 0;
}

void gui::scroll::draw(const widget *focus, const widget *input) const
{
    int thumb_h = area.h - 4;
    int thumb_y = area.y + 2;
    int m;

    // Compute the size and position of the scroll thumb.

    if ((m = std::max(child_h - area.h, 0)) > 0)
    {
        thumb_h  = (area.h - 4) * (area.h - 4) / child_h;
        thumb_y += (area.h - 4 - thumb_h) * (m - child_d) / m;
    }

    // Draw the scroll bar.

    glPushAttrib(GL_ENABLE_BIT);
    {
        glDisable(GL_TEXTURE_2D);

        glBegin(GL_QUADS);
        {
            back_color(focus);

            glVertex2i(area.R() - scroll_w, area.B());
            glVertex2i(area.R(),            area.B());
            glVertex2i(area.R(),            area.T());
            glVertex2i(area.R() - scroll_w, area.T());
        }
        glEnd();

        // Draw the scroll thumb.

        glBegin(GL_QUADS);
        {
            fore_color();

            glVertex2i(area.R() - scroll_w + 2, thumb_y);
            glVertex2i(area.R()            - 2, thumb_y);
            glVertex2i(area.R()            - 2, thumb_y + thumb_h);
            glVertex2i(area.R() - scroll_w + 2, thumb_y + thumb_h);
        }
        glEnd();
    }
    glPopAttrib();

    // Draw the children, clipped by the area of the scroll.

    glPushAttrib(GL_ENABLE_BIT);
    glPushMatrix();
    {
        GLdouble L[4] = { +1,  0, 0, -area.L() };
        GLdouble R[4] = { -1,  0, 0, +area.R() };
        GLdouble B[4] = {  0, +1, 0, -area.B() };
        GLdouble T[4] = {  0, -1, 0, +area.T() };

        glEnable(GL_CLIP_PLANE0);
        glEnable(GL_CLIP_PLANE1);
        glEnable(GL_CLIP_PLANE2);
        glEnable(GL_CLIP_PLANE3);

        glClipPlane(GL_CLIP_PLANE0, L);
        glClipPlane(GL_CLIP_PLANE1, R);
        glClipPlane(GL_CLIP_PLANE2, B);
        glClipPlane(GL_CLIP_PLANE3, T);

        glTranslatef(0, float(child_d), 0);
        tree::draw(focus, input);
    }
    glPopMatrix();
    glPopAttrib();
}

//-----------------------------------------------------------------------------
// File selection list.

void gui::finder::refresh()
{
    // Delete all child nodes.

    for (widget_v::const_iterator i = child.begin(); i != child.end(); ++i)
        delete (*i);

    child.clear();

    // List all files and subdirectories in the current directory.

    strset dirs;
    strset regs;

    ::data->list(cwd, dirs, regs);

    // Add a new file button for each file and subdirectory.

    add(new finder_dir("..", this));

    for (strset::iterator i = dirs.begin(); i != dirs.end(); ++i)
        add(new finder_dir(*i, this));

    for (strset::iterator i = regs.begin(); i != regs.end(); ++i)
        add(new finder_reg(*i, this));

    add(new gui::filler());

    // Find the total height of all children.

    child_d = 0;
    child_h = 0;

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        child_h += (*i)->get_h();

    // Lay out this new widget hierarchy.

    laydn(area.x, area.y, area.w, area.h);
}

void gui::finder::set_dir(const std::string& name)
{
    if (name == "..")
    {
        std::string::size_type s = cwd.rfind("/");

        // Remove the trailing directory from the CWD.

        if (s != std::string::npos)
            cwd.erase(s);
        else
            cwd.erase( );
    }
    else
    {
        // Append the named directory to the CWD.

        if (cwd.empty())
            cwd = name;
        else
            cwd = cwd + "/" + name;
    }

    // Update the directory listing.

    refresh();
}

void gui::finder::set_reg(const std::string& name)
{
    if (state)
        state->value(cwd + "/" + name);
}

//-----------------------------------------------------------------------------
// File selection list element.

gui::finder_elt::finder_elt(std::string s, gui::finder *w) :
    button(s, string::mono, -1, 0), target(w)
{
    // Pack these slighly closer together than normal buttons.

    area.w = text->w() + font->size() * 2;
    area.h = text->h() + font->size() / 2;
}

//-----------------------------------------------------------------------------
// Horizontal group of equally-sized widgets.

void gui::harray::layup()
{
    // Find the widest child width and the highest child height.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(), get_w());
        area.h = std::max((*i)->get_h(), get_h());
    }

    // Total width is the widest child width times the child count.

    area.w *= int(child.size());
}

void gui::harray::laydn(int x, int y, int w, int h)
{
    int n = int(child.size()), c = 0;

    widget::laydn(x, y, w, h);

    // Distribute horizontal space evenly to all children.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i, ++c)
    {
        int x0 = x + (c    ) * w / n;
        int x1 = x + (c + 1) * w / n;

        (*i)->laydn(x0, y, x1 - x0, h);
    }
}

//-----------------------------------------------------------------------------
// Vertical group of equally sized widgets.

void gui::varray::layup()
{
    // Find the widest child width and the highest child height.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(), get_w());
        area.h = std::max((*i)->get_h(), get_h());
    }

    // Total height is the heighest child height times the child count.

    area.h *= int(child.size());
}

void gui::varray::laydn(int x, int y, int w, int h)
{
    int n = int(child.size()), c = n - 1;

    widget::laydn(x, y, w, h);

    // Distribute vertical space evenly to all children.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i, --c)
    {
        int y0 = y + (c    ) * h / n;
        int y1 = y + (c + 1) * h / n;

        (*i)->laydn(x, y0, w, y1 - y0);
    }
}

//-----------------------------------------------------------------------------
// Horizontal group of widgets.

void gui::hgroup::layup()
{
    // Find the highest child height.  Sum the child widths.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w =         ((*i)->get_w() + get_w());
        area.h = std::max((*i)->get_h(),  get_h());
    }
}

void gui::hgroup::laydn(int x, int y, int w, int h)
{
    int c = 0, excess = w - area.w;

    widget::laydn(x, y, w, h);

    // Count the number of expanding children.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        if ((*i)->exp_w())
            c++;

    // Give children requested space.  Distribute excess among the expanders.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        int dx = (*i)->get_w() + ((*i)->exp_w() ? (excess / c) : 0);

        (*i)->laydn(x, y, dx, h);

        x += dx;
    }
}

//-----------------------------------------------------------------------------
// Vertical group of widgets.

void gui::vgroup::layup()
{
    // Find the widest child width.  Sum the child heights.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(),  get_w());
        area.h =         ((*i)->get_h() + get_h());
    }
}

void gui::vgroup::laydn(int x, int y, int w, int h)
{
    int c = 0, excess = h - area.h;

    widget::laydn(x, y, w, h);

    // Count the number of expanding children.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        if ((*i)->exp_h())
            c++;

    // Give children requested space.  Distribute excess among the expanders.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        int dy = (*i)->get_h() + ((*i)->exp_h() ? (excess / c) : 0);

        (*i)->laydn(x, y + h - dy, w, dy);

        y -= dy;
    }
}

//-----------------------------------------------------------------------------
// Tabb selector panel.

void gui::option::layup()
{
    // Find the largest child width extent.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(), get_w());
        area.h = std::max((*i)->get_h(), get_h());
    }
}

void gui::option::laydn(int x, int y, int w, int h)
{
    widget::laydn(x, y, w, h);

    // Give children all available space.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        (*i)->laydn(x, y, w, h);
}

gui::widget *gui::option::enter(int x, int y)
{
    return child[index]->enter(x, y);
}

void gui::option::draw(const widget *focus, const widget *input) const
{
    child[index]->draw(focus, input);
}

//-----------------------------------------------------------------------------
// Padding frame.

void gui::frame::layup()
{
    // Find the largest child width extent.  Pad it.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
    {
        (*i)->layup();

        area.w = std::max((*i)->get_w(), get_w());
        area.h = std::max((*i)->get_h(), get_h());
    }

    area.w += border * 2;
    area.h += border * 2;
}

void gui::frame::laydn(int x, int y, int w, int h)
{
    widget::laydn(x, y, w, h);

    // Give children all available space.

    for (widget_v::iterator i = child.begin(); i != child.end(); ++i)
        (*i)->laydn(x + border,
                    y + border,
                    w - border * 2,
                    h - border * 2);
}

void gui::frame::draw(const widget *focus, const widget *input) const
{
    glPushAttrib(GL_ENABLE_BIT);
    {
        glDisable(GL_TEXTURE_2D);

        back_color(focus);

        // Draw the background.

        glBegin(GL_QUADS);
        {
            const int Lo = area.L();
            const int Li = area.L() + border;
            const int Ri = area.R() - border;
            const int Ro = area.R();
            const int To = area.T();
            const int Ti = area.T() - border;
            const int Bi = area.B() + border;
            const int Bo = area.B();

            glVertex2i(Lo, To);
            glVertex2i(Lo, Bo);
            glVertex2i(Li, Bi);
            glVertex2i(Li, Ti);

            glVertex2i(Ri, Ti);
            glVertex2i(Ri, Bi);
            glVertex2i(Ro, Bo);
            glVertex2i(Ro, To);

            glVertex2i(Lo, To);
            glVertex2i(Li, Ti);
            glVertex2i(Ri, Ti);
            glVertex2i(Ro, To);

            glVertex2i(Li, Bi);
            glVertex2i(Lo, Bo);
            glVertex2i(Ro, Bo);
            glVertex2i(Ri, Bi);
        }
        glEnd();
    }
    glPopAttrib();

    tree::draw(focus, input);
}

//-----------------------------------------------------------------------------
// Top level dialog.

gui::dialog::dialog()
{
    root  = 0;
    focus = 0;
    input = 0;
}

gui::dialog::~dialog()
{
    if (root) delete root;
}

void gui::dialog::point(int x, int y)
{
    // Dragging outside of a widget should not defocus it.

    if (focus && focus->pressed())
        focus->point(x, y);
    else
        focus = root->enter(x, y);

    last_x = x;
    last_y = y;
}

void gui::dialog::click(int m, bool d)
{
    // Click any focused widget.  Shift the input focus there.

    if (focus)
        input = focus->click(last_x, last_y, m, d);

    // Activation may have resulted in widget deletion.  Confirm the focus.

    if (d == false)
        focus = root->enter(last_x, last_y);
}

void gui::dialog::keybd(int c, int k, int m)
{
    if (input)
        input->keybd(c, k, m);
}

void gui::dialog::show()
{
    if (root) root->show();
}

void gui::dialog::hide()
{
    if (root) root->hide();

    input = 0;
    focus = 0;
}

void gui::dialog::draw() const
{
    if (root)
    {
        glPushAttrib(GL_ENABLE_BIT);
        {
            glMatrixMode(GL_TEXTURE);
            {
                glLoadIdentity();
            }
            glMatrixMode(GL_PROJECTION);
            {
                glLoadIdentity();
/* TODO: GUI must take its projection
                user->mult_P();
*/
            }
            glMatrixMode(GL_MODELVIEW);
            {
                glLoadIdentity();
/* TODO: GUI must know its own transform
                host->gui_view();
*/
            }

            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);

            glEnable(GL_BLEND);
            glEnable(GL_DEPTH_CLAMP_NV); // TODO: better

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            root->draw(focus, input);

            glBegin(GL_LINES);
            {
                int s = 10;

                glColor4f(1.0f, 1.0f, 0.0f, 1.0f);

                glVertex2i(last_x - s, last_y - s);
                glVertex2i(last_x + s, last_y + s);
                glVertex2i(last_x - s, last_y + s);
                glVertex2i(last_x + s, last_y - s);
            }
            glEnd();
        }
        glPopAttrib();
    }
}

//-----------------------------------------------------------------------------