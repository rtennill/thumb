//  Copyright (C) 2007 Robert Kooima
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

#ifndef GEOMAP
#define GEOMAP

#include "texture.hpp"
#include "opengl.hpp"

//-----------------------------------------------------------------------------

namespace uni
{
    //-------------------------------------------------------------------------
    // Geomap tile

    class tile
    {
        enum { dead_state, wait_state, live_state } state;

        tile  *P[4];
        double b[6];
        int d, i, j;
        double L;
        double R;
        double B;
        double T;

        GLuint object;

        bool visible(const double *, const double *, const double *);

    public:

        tile(std::string&, int, int, int, int, int, int,
             double, double, double, double, double, double);
       ~tile();

        void search();

        void ready(GLuint);
        void eject();

        void draw(const double *, const double *, const double *);
        void wire();
    };

    //-------------------------------------------------------------------------
    // Geomap

    class geomap
    {
        std::string name;

        int w;
        int h;
        int c;
        int b;
        int s;

        GLfloat hoff;
        GLfloat hscl;

        tile *P;

        // Tile LRU queue
        // Tile draw stack
        // File response queue
        // static File request queue

    public:

        geomap(std::string, int, int, int, int, int, 
               double, double, double, double, double, double);
       ~geomap();

        void draw(const double *, const double *, const double *);
        void wire();
    };
}

//-----------------------------------------------------------------------------

#endif
