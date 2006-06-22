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

#include <SDL.h>

#include "joint.hpp"
#include "info.hpp"

//-----------------------------------------------------------------------------

mode::info::info(ops::scene& s) : mode(s), gui(s)
{
}

//-----------------------------------------------------------------------------

void mode::info::enter()
{
    gui.show();
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,
                        SDL_DEFAULT_REPEAT_INTERVAL);
}

void mode::info::leave()
{
    SDL_EnableKeyRepeat(0, 0);
    gui.hide();
}

//-----------------------------------------------------------------------------

bool mode::info::point(const float[3], const float[3], int x, int y)
{
    gui.point(x, y);
    return false;
}

bool mode::info::click(int b, bool d)
{
    if (b == 1)
    {
        gui.click(d);
        return true;
    }
    return false;
}

bool mode::info::keybd(int k, bool d, int c)
{
    if (d) gui.keybd(k, c);
    return true;
}

//-----------------------------------------------------------------------------

void mode::info::draw() const
{
    glPushAttrib(GL_ENABLE_BIT);
    {
        // Draw the scene.

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_NORMALIZE);
        glEnable(GL_LIGHTING);

        scene.draw(ent::flag_fill | ent::flag_line);

        // Draw the GUI.

        gui.draw();
    }
    glPopAttrib();
}

//-----------------------------------------------------------------------------
