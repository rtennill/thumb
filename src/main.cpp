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
#include <cstdlib>

#include <SDL.h>

#include "main.hpp"
#include "host.hpp"
#include "util.hpp"
#include "tracker.hpp"
#include "opengl.hpp"
#include "demo.hpp"
#include "data.hpp"
#include "conf.hpp"
#include "glob.hpp"
#include "lang.hpp"
#include "view.hpp"
#include "perf.hpp"

//-----------------------------------------------------------------------------
// Global application state.

app::conf *conf;
app::data *data;
app::prog *prog;
app::glob *glob;
app::view *view;
app::lang *lang;
app::host *host;
app::perf *perf;

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

static void position(int x, int y)
{
    char buf[32];

    // SDL looks to the environment for window position.

#ifdef _WIN32
    sprintf(buf, "SDL_VIDEO_WINDOW_POS=%d,%d", x, y);
    putenv(buf);
#else
    sprintf(buf, "%d,%d", x, y);
    setenv("SDL_VIDEO_WINDOW_POS", buf, 1);
#endif
}

static void video()
{
    // Look up the video mode parameters.

    int m = host->get_window_m() | SDL_OPENGL;
    int x = host->get_window_x();
    int y = host->get_window_y();
    int w = host->get_window_w();
    int h = host->get_window_h();

    // TODO: fold these into host config.

    int b = conf->get_i("window_b");

    if (conf->get_i("window_fullscreen")) m |= SDL_FULLSCREEN;
    if (conf->get_i("window_noframe"))    m |= SDL_NOFRAME;

    if (m & SDL_NOFRAME)
        SDL_ShowCursor(SDL_DISABLE);

    // Initialize the video.

    position(x, y);

    if (SDL_SetVideoMode(w, h, b, m) == 0)
        throw std::runtime_error(SDL_GetError());
    
    SDL_WM_SetCaption("Thumb", "Thumb");

    // Initialize the OpenGL state.

    ogl::init();
}

static void init(std::string& h)
{
    // Initialize data access and configuration.

    data = new app::data(DEFAULT_DATA_FILE);
    conf = new app::conf(DEFAULT_CONF_FILE);

    // Initialize language and host configuration.

    std::string lang_conf = conf->get_s("lang_file");
    std::string host_conf = conf->get_s("host_file");

    lang = new app::lang(lang_conf.empty() ? DEFAULT_LANG_FILE : lang_conf);
    host = new app::host(host_conf.empty() ? DEFAULT_HOST_FILE : host_conf, h);

    // Initialize the OpenGL context.

    video();

    // Initialize the OpenGL state and application.

    view = new app::view(host->get_window_w(),
                         host->get_window_h());
    glob = new app::glob();
    perf = new app::perf();
    prog = new demo();

    // Initialize the controllers.

    joy = SDL_JoystickOpen(conf->get_i("joystick"));

    int tracker_key = conf->get_i("tracker_key");
    int control_key = conf->get_i("control_key");

    if (tracker_key == 0)
        tracker_key = DEFAULT_TRACKER_KEY;
    if (control_key == 0)
        control_key = DEFAULT_CONTROL_KEY;

    tracker_init(tracker_key, control_key);
}

static void fini()
{
    tracker_fini();

    if (joy) SDL_JoystickClose(joy);

    if (prog) delete prog;
    if (perf) delete perf;
    if (glob) delete glob;
    if (view) delete view;
    if (host) delete host;
    if (lang) delete lang;
    if (conf) delete conf;
    if (data) delete data;
}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    std::string tag(argc > 1 ? argv[1] : DEFAULT_TAG);

    try
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) == 0)
        {
            SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     5);
            SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   5);
            SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    5);
            SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,  16);
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

            SDL_EnableUNICODE(1);

            init(tag);
            {
                host->loop();
            }
            fini();

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
