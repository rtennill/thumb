//  Copyright (C) 2005-2011 Robert Kooima
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

#include <cmath>

#include <ogl-opengl.hpp>

#include <etc-math.hpp>
#include <app-data.hpp>
#include <app-host.hpp>
#include <app-user.hpp>
#include <app-conf.hpp>
#include <app-prog.hpp>
#include <app-event.hpp>
#include <app-frustum.hpp>
#include <app-default.hpp>

#include "math3d.h"
#include "panoview.hpp"

//------------------------------------------------------------------------------

panoview::panoview(const std::string& exe,
                   const std::string& tag) : scm_viewer(exe, tag),
    min_zoom(-2.0),
    max_zoom( 0.6),
    debug_zoom(false)
{
    curr_zoom    = 0.0;
    drag_zooming = false;
    drag_looking = false;
}

panoview::~panoview()
{
}

//------------------------------------------------------------------------------

void panoview::draw(int frusi, const app::frustum *frusp, int chani)
{
    glClearColor(0.4f, 0.4f, 0.4f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (model)
    {
        const double *M = ::user->get_M();

        if (debug_zoom)
            model->set_zoom(  0.0,   0.0,   -1.0, pow(10.0, curr_zoom));
        else
            model->set_zoom(-M[8], -M[9], -M[10], pow(10.0, curr_zoom));
    }

    channel = chani;

    scm_viewer::draw(frusi, frusp, chani);
    scm_viewer::over(frusi, frusp, chani);
}

//------------------------------------------------------------------------------

bool panoview::process_event(app::event *E)
{
    if (!scm_viewer::process_event(E))
    {
        switch (E->get_type())
        {
            case E_CLICK: return pan_click(E);
            case E_POINT: return pan_point(E);
            case E_TICK:  return pan_tick(E);
            case E_KEY:   return pan_key(E);
        }
    }
    return false;
}

//------------------------------------------------------------------------------

bool panoview::pan_point(app::event *E)
{
    if (const app::frustum *overlay = ::host->get_overlay())
        overlay->pointer_to_2D(E, curr_x, curr_y);

    return false;
}

bool panoview::pan_click(app::event *E)
{
    if (E->data.click.b == 0)
        drag_looking = E->data.click.d;
    if (E->data.click.b == 2)
        drag_zooming = E->data.click.d;

    drag_x    = curr_x;
    drag_y    = curr_y;
    drag_zoom = curr_zoom;

    return true;
}

bool panoview::pan_tick(app::event *E)
{
    float dt = E->data.tick.dt / 1000.0;

    if (drag_zooming)
    {
        curr_zoom = drag_zoom + (curr_y - drag_y) / 500.0f;

        if (curr_zoom < min_zoom) curr_zoom = min_zoom;
        if (curr_zoom > max_zoom) curr_zoom = max_zoom;
    }
    if (drag_looking)
    {
        int dx = curr_x - drag_x;
        int dy = curr_y - drag_y;

        ::user->look(-dx * dt, dy * dt);
    }
    return false;
}

bool panoview::pan_key(app::event *E)
{
    if (E->data.key.d)
        switch (E->data.key.k)
        {
        case 280: goto_next(); return true;
        case 281: goto_prev(); return true;
        case 285: debug_zoom  = !debug_zoom;  return true;
        }

    return false;
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    try
    {
        app::prog *P;

        P = new panoview(argv[0], std::string(argc > 1 ? argv[1] : DEFAULT_TAG));
        P->run();

        delete P;
    }
    catch (std::exception& e)
    {
        fprintf(stderr, "Exception: %s\n", e.what());
    }
    return 0;
}

//------------------------------------------------------------------------------
