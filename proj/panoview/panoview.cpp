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

#include <app-host.hpp>
#include <app-user.hpp>
#include <app-prog.hpp>
#include <app-event.hpp>
#include <app-frustum.hpp>

#include "math3d.h"
#include "panoview.hpp"

//------------------------------------------------------------------------------

// panoview::panoview(const std::string& tag) :
//    app::prog(tag), C(512), L(C, 16, 4, 256)
// {
//    panoL = C.add_file("/home/rlk/Data/pan/test4.tif");
// }

panoview::panoview(const std::string& tag) :
    app::prog(tag), C(256), L(C, 16, 3, 512)
{
    int t;
    
//    panoL = C.add_file("/home/rlk/Data/pan/Taliesin-Garden-13-L-Cube.tif");
    panoL = C.add_file("/Users/rlk/Data/pan/Taliesin-Garden-13-L-Cube.tif");

    C.get_page(panoL, 0, 0, t);
    C.get_page(panoL, 1, 0, t);
    C.get_page(panoL, 2, 0, t);
    C.get_page(panoL, 3, 0, t);
    C.get_page(panoL, 4, 0, t);
    C.get_page(panoL, 5, 0, t);
}

panoview::~panoview()
{
}

//------------------------------------------------------------------------------

ogl::range panoview::prep(int frusc, const app::frustum *const *frusv)
{
    const double *P = frusv[0]->get_P();
    const double *M = ::user->get_M();
    const int     w = ::host->get_buffer_w();
    const int     h = ::host->get_buffer_h();

    double V[16];

    minvert(V, M);
    L.prep (P, V, w, h);

    return ogl::range(0.001, 10.0);
}

void panoview::lite(int frusc, const app::frustum *const *frusv)
{
}

void panoview::draw(int frusi, const app::frustum *frusp)
{
    const double *P =  frusp->get_P();
    const double *M = ::user->get_M();

    double V[16];

    // glClearColor(0.4f, 0.4f, 0.4f, 0.0f);
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
 
    glClear(GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

    minvert(V, M);
    L.draw (P, V, panoL);
}

//------------------------------------------------------------------------------

bool panoview::process_event(app::event *E)
{
//    if (E->get_type() == E_TICK)
//    {
//        ::user->turn(E->data.tick.dt / 75.0,
//                     E->data.tick.dt / 70.0, 0.0);
//        return false;
//    }
//    else
        return prog::process_event(E);
}

//------------------------------------------------------------------------------
