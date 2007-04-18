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

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>

#include <SDL.h>

#include "util.hpp"
#include "opengl.hpp"
#include "demo.hpp"
#include "main.hpp"

#define JIFFY (1000 / 60)

//-----------------------------------------------------------------------------
// Global application state.

app::conf *conf;
app::data *data;
app::prog *prog;
app::lang *lang;
app::font *sans;
app::font *mono;
app::glob *glob;
app::view *view;

//-----------------------------------------------------------------------------
// C-callable global state accessors.

extern "C" {

    const void *load_dat(const char *name, size_t *size)
    {
        return ::data->load_dat(name, size);
    }

    void free_dat(const char *name)
    {
        ::data->free_dat(name);
    }

    void load_tex(const char *name)
    {
        ::glob->load_texture(name);
    }

    void free_tex(const char *name)
    {
        ::glob->free_texture(name);
    }
}

//-----------------------------------------------------------------------------
// Keyboard state expression queryables.

double get_key(int i)
{
    int    n;
    Uint8 *k = SDL_GetKeyState(&n);

    if (0 <= i && i < n && k[i])
        return 1.0;
    else
        return 0.0;
}

//-----------------------------------------------------------------------------
// System time expression queryables.

static int tick = 0;
static int tock = 0;

double get_time()
{
    return (tock - tick) / 1000.0;
}

void clr_time()
{
    tick = tock;
}

//-----------------------------------------------------------------------------
// Joystick input expression queryables.

static SDL_Joystick *joy = NULL;

double get_btn(int i)
{
    if (joy)
        return SDL_JoystickGetButton(joy, i);
    else
        return 0.0;
}

double get_joy(int i)
{
    if (joy)
        return SDL_JoystickGetAxis(joy, i) / 32768.0;
    else
        return 0.0;
}

//-----------------------------------------------------------------------------
// Collision group expression queryables.

static unsigned int bits = 0;

double get_trg(unsigned int i)
{
    if (bits & (1 << i))
        return 1.0;
    else
        return 0.0;
}

void set_trg(unsigned int b) { bits |= b; }
void clr_trg()               { bits  = 0; }

//-----------------------------------------------------------------------------

static std::string get_loc()
{
    // Check for a locale setting in the config file.

    std::string loc = conf->get_s("lang");

    if (loc.size() > 0)
        return loc;

    // Check for a locale setting in the environment.

    if (getenv("LANG"))
        return getenv("LANG");

    // If all else fails, use the default.

    return "en_US";
}    

static void stat()
{
    static float t0 = 0;
    static float t1 = 0;
    static float tt = 0;
    static int   n  = 0;

    // Accumulate passing time.

    t1 = SDL_GetTicks() / 1000.0f;
    tt = tt + t1 - t0;
    t0 = t1;

    // Count frames and report frames per second.

    n++;

    if (tt > 0.25f)
    {
        std::ostringstream str;

        str << int(n / tt);

        SDL_WM_SetCaption(str.str().c_str(),
                          str.str().c_str());
        tt = 0;
        n  = 0;
    }
}

static bool loop(std::string& conf_file, std::string& lang_file)
{
    int w, h, b, m;

    SDL_Event e;

    // While there are available events, dispatch event handlers.

    while (SDL_PollEvent(&e))
        switch (e.type)
        {
        case SDL_QUIT:
            if (prog) delete prog;
            if (glob) delete glob;
            if (view) delete view;
            if (mono) delete mono;
            if (sans) delete sans;
            if (data) delete data;
            if (lang) delete lang;
            if (conf) delete conf;

            if (joy) SDL_JoystickClose(joy);

            return false;

        case SDL_USEREVENT:
            if (prog) delete prog;
            if (glob) delete glob;
            if (view) delete view;
            if (mono) delete mono;
            if (sans) delete sans;
            if (data) delete data;
            if (lang) delete lang;
            if (conf) delete conf;

            if (joy) SDL_JoystickClose(joy);

            conf = new app::conf(conf_file);
            lang = new app::lang(lang_file, get_loc());
            data = new app::data();
            sans = new app::font(conf->get_s("sans_font"),
                                 conf->get_i("sans_size"));
            mono = new app::font(conf->get_s("mono_font"),
                                 conf->get_i("mono_size"));

            // Look up the video mode parameters.

            w = conf->get_i("window_w");
            h = conf->get_i("window_h");
            b = conf->get_i("window_b");
            m = SDL_OPENGL;

            if (conf->get_i("window_fullscreen")) m |= SDL_FULLSCREEN;
            if (conf->get_i("window_noframe"))    m |= SDL_NOFRAME;

            // Initialize the video.

            if (SDL_SetVideoMode(w, h, b, m) == 0)
                throw std::runtime_error(SDL_GetError());

            ogl::init();

            // Initialize the demo.

            prog = new demo();
            glob = new app::glob();
            view = new app::view(w, h, conf->get_f("view_near"),
                                       conf->get_f("view_far"),
                                       conf->get_f("view_zoom"));

            joy  = SDL_JoystickOpen(conf->get_i("joystick"));

            break;

        case SDL_MOUSEMOTION:     prog->point(e.motion.x, e.motion.y);  break;
        case SDL_MOUSEBUTTONDOWN: prog->click(e.button.button,  true);  break;
        case SDL_MOUSEBUTTONUP:   prog->click(e.button.button,  false); break;
        case SDL_KEYDOWN:         prog->keybd(e.key.keysym.sym, true,
                                              e.key.keysym.unicode);    break;
        case SDL_KEYUP:           prog->keybd(e.key.keysym.sym, false,
                                              e.key.keysym.unicode);    break;
        }

    // If a jiffy has expired, call the timer method.

    while (SDL_GetTicks() - tock >= JIFFY)
    {
        tock += JIFFY;
        prog->timer(JIFFY / 1000.0f);
    }

    // Draw the scene and display statistics. 

    prog->draw();
    stat();

    return true;
}

int main(int argc, char *argv[])
{
    std::string conf_file(argc > 1 ? argv[1] : DEFAULT_CONF_FILE);
    std::string lang_file(argc > 2 ? argv[2] : DEFAULT_LANG_FILE);

    try
    {
        ent::entity::phys_init();

        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == 0)
        {
            SDL_Event e = { SDL_USEREVENT };

            SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  16);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            SDL_EnableUNICODE(1);
            SDL_PushEvent(&e);

            while (loop(conf_file, lang_file))
                SDL_GL_SwapBuffers();

            SDL_Quit();
        }
        else throw std::runtime_error(SDL_GetError());
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}

//-----------------------------------------------------------------------------
