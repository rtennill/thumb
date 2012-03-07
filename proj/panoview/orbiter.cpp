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
#include "orbiter.hpp"

//------------------------------------------------------------------------------

orbiter::orbiter(const std::string& exe,
                 const std::string& tag) : sph_viewer(exe, tag)
{
    orbit_plane[0] = 0.0;
    orbit_plane[1] = 1.0;
    orbit_plane[2] = 0.0;
    orbit_speed    = 0.0;

    position[0]    = 0.0;
    position[1]    = 0.0;
    position[2]    = 1.0;
    altitude       = 2.0 * get_radius();

    view_x[0]      = 1.0;
    view_x[1]      = 0.0;
    view_x[2]      = 0.0;

    view_y[0]      = 0.0;
    view_y[1]      = 1.0;
    view_y[2]      = 0.0;

    drag_move  = false;
    drag_look  = false;
    drag_dive  = false;
    drag_light = false;

    crater = new sph_crater("moon.xml");
}

orbiter::~orbiter()
{
    delete crater;
}

//------------------------------------------------------------------------------

void orbiter::tick_look(double dt)
{
    const double *M = ::user->get_M();

    double dx = point_v[0] - click_v[0];
    double dy = point_v[1] - click_v[1];

    double X[16];
    double Y[16];
    double R[16];
    double t[3];

    mrotate(X, M,         10.0 * dy * dt);
    mrotate(Y, position, -10.0 * dx * dt);
    mmultiply(R, X, Y);

    vtransform(t, R, view_x);
    vnormalize(view_x, t);

    vtransform(t, R, view_y);
    vnormalize(view_y, t);
}

void orbiter::tick_move(double dt)
{
    if (click_v[0] != point_v[0] ||
        click_v[1] != point_v[1] ||
        click_v[2] != point_v[2])
    {
        double h = (altitude - get_radius()) / get_radius();

        double d = vdot(click_v, point_v);
        double a = acos(d);

        if (fabs(a) > 0.00)
        {
            const double *M = ::user->get_M();
            double t[3];
            double u[3];

            vcrs(t, click_v, point_v);

            vtransform(u, M, t);
            vnormalize(orbit_plane, u);
        }
        orbit_speed = 10.0 * a * h;
    }
    else
    {
        orbit_speed = 0.0;
    }
}

void orbiter::tick_dive(double dt)
{
    altitude += 2.0 * get_radius() * (point_v[1] - click_v[1]) * dt;
}

//------------------------------------------------------------------------------

void orbiter::tick(double dt)
{
    double M[16];
    double t[3];

    if (drag_move) tick_move(dt);
    if (drag_look) tick_look(dt);
    if (drag_dive) tick_dive(dt);

    // Move the position and view orientation along the current orbit.

    double R[16];

    mrotate(R, orbit_plane, orbit_speed * dt);

    vtransform(t, R, view_x);
    vnormalize(view_x, t);

    vtransform(t, R, view_y);
    vnormalize(view_y, t);

    vtransform(t, R, position);
    vnormalize(position, t);

    // Orthonormalize the view orientation basis.

    double view_z[3];

    vcrs(view_z, view_x, view_y);
    vcrs(view_y, view_z, view_x);

    mbasis(M, view_x, view_y, view_z);

    // Update the user transform.

    M[12] = position[0] * altitude;
    M[13] = position[1] * altitude;
    M[14] = position[2] * altitude;

    ::user->set_M(M);
}

void orbiter::draw(int frusi, const app::frustum *frusp, int chani)
{
   sph_viewer::draw(frusi, frusp, chani);

    frusp->draw();
   ::user->draw();

    crater->draw(position, get_radius(), altitude);
}

//------------------------------------------------------------------------------

bool orbiter::process_event(app::event *E)
{
    if (!sph_viewer::process_event(E))
    {
        switch (E->get_type())
        {
            case E_CLICK: return pan_click(E);
            case E_POINT: return pan_point(E);
            case E_KEY:   return pan_key(E);
            case E_TICK:  tick(E->data.tick.dt / 1000.0);
        }
    }
    return false;
}

//------------------------------------------------------------------------------

bool orbiter::pan_point(app::event *E)
{
    double M[16];

    // Determine the point direction of the given quaternion.

    quat_to_mat(M, E->data.point.q);

    point_v[0] = -M[ 8];
    point_v[1] = -M[ 9];
    point_v[2] = -M[10];

    return false;
}

bool orbiter::pan_click(app::event *E)
{
    const int  b = E->data.click.b;
    const int  m = E->data.click.m;
    const bool d = E->data.click.d;

    vcpy(click_v, point_v);

    if (b == 0 && m == 0) drag_move  = d;
    if (b == 2 && m == 0) drag_look  = d;
    if (b == 0 && m & 1)  drag_dive  = d;
    if (b == 2 && m & 1)  drag_light = d;

    return true;
}

bool orbiter::pan_key(app::event *E)
{
    if (E->data.key.d)
        switch (E->data.key.k)
        {
        case 280: goto_next(); return true;
        case 281: goto_prev(); return true;
        }

    return false;
}

//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    try
    {
        app::prog *P;

        P = new orbiter(argv[0], std::string(argc > 1 ? argv[1] : DEFAULT_TAG));
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