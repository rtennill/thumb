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

#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <png.h>

#include "geomap.hpp"
#include "matrix.hpp"
#include "util.hpp"

static GLuint load_png(std::string base, int d, int i, int j)
{
    GLuint o = 0;

    // Construct the file name.

    std::ostringstream name;

    name << base
         << "-" << std::hex << std::setfill('0') << std::setw(2) << d
         << "-" << std::hex << std::setfill('0') << std::setw(2) << i
         << "-" << std::hex << std::setfill('0') << std::setw(2) << j << ".png";

    // Initialize all PNG import data structures.

    png_structp rp = 0;
    png_infop   ip = 0;
    png_bytep  *bp = 0;
    FILE       *fp = 0;

    if (!(fp = fopen(name.str().c_str(), "rb")))
        throw std::runtime_error("Failure opening PNG file");

    if (!(rp = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0)))
        throw std::runtime_error("Failure creating PNG read structure");

    if (!(ip = png_create_info_struct(rp)))
        throw std::runtime_error("Failure creating PNG info structure");

    // Enable the default PNG error handler.

    if (setjmp(png_jmpbuf(rp)) == 0)
    {
        GLenum type_tag[2] = {
            GL_UNSIGNED_BYTE,
            GL_UNSIGNED_SHORT
        };
        GLenum form_tag[2][4] = {
            { GL_LUMINANCE,   GL_LUMINANCE_ALPHA,     GL_RGB,   GL_RGBA   },
            { GL_LUMINANCE16, GL_LUMINANCE16_ALPHA16, GL_RGB16, GL_RGBA16 },
        };

        // Read the PNG header.

        png_init_io (rp, fp);
        png_read_png(rp, ip, PNG_TRANSFORM_PACKING |
                             PNG_TRANSFORM_SWAP_ENDIAN, 0);
        
        // Extract image properties.

        GLsizei w = GLsizei(png_get_image_width (rp, ip));
        GLsizei h = GLsizei(png_get_image_height(rp, ip));
        GLsizei b = GLsizei(png_get_bit_depth   (rp, ip)) / 8;
        GLsizei c = 1;
        GLsizei d = 0;

        if (w & (w - 1)) d = 1;
        if (h & (h - 1)) d = 1;

        switch (png_get_color_type(rp, ip))
        {
        case PNG_COLOR_TYPE_GRAY:       c = 1; break;
        case PNG_COLOR_TYPE_GRAY_ALPHA: c = 2; break;
        case PNG_COLOR_TYPE_RGB:        c = 3; break;
        case PNG_COLOR_TYPE_RGB_ALPHA:  c = 4; break;
        default: throw std::runtime_error("Unsupported PNG color type");
        }

        GLenum fi   = form_tag[b - 1][c - 1];
        GLenum fe   = form_tag[0    ][c - 1];
        GLenum type = type_tag[b - 1];

        // Read the pixel data.

        if ((bp = png_get_rows(rp, ip)))
        {
            glGenTextures(1, &o);

            // Initialize the texture object.

            glBindTexture(GL_TEXTURE_2D, o);
            glTexImage2D (GL_TEXTURE_2D, 0, fi, w, h, d, fe, type, 0);

            OGLCK();

            // Copy all rows to the new texture.

            for (GLsizei i = 0, j = h - 1; j >= 0; ++i, --j)
                glTexSubImage2D(GL_TEXTURE_2D, 0, -d, i - d, w, 1,
                                fe, type, bp[j]);

            OGLCK();

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        }
    }

    // Release all resources.

    png_destroy_read_struct(&rp, &ip, 0);
    fclose(fp);

    return o;
}

//-----------------------------------------------------------------------------

uni::tile::tile(std::string& name,
                int w, int h, int s,
                int x, int y, int d,
                double r0, double r1,
                double _L, double _R, double _B, double _T) :
    state(dead_state), d(d), object(0)
{
    int S = s << d;

    // Compute the mipmap file indices.

    i = y / S;
    j = x / S;

    // Compute the texture boundries.

    L = (_L + (_R - _L) * (x    ) / w);
    R = (_L + (_R - _L) * (x + S) / w);
    T = (_T + (_B - _T) * (y    ) / h);
    B = (_T + (_B - _T) * (y + S) / h);

    // Create subtiles as necessary.

    P[0] = 0;
    P[1] = 0;
    P[2] = 0;
    P[3] = 0;

    if (d > 0)
    {
        const int X = (x + S / 2);
        const int Y = (y + S / 2);

        /* Has children? */ P[0] = new tile(name, w, h, s, x, y, d-1, r0, r1, _L, _R, _B, _T);
        if (         Y < h) P[1] = new tile(name, w, h, s, x, Y, d-1, r0, r1, _L, _R, _B, _T);
        if (X < w         ) P[2] = new tile(name, w, h, s, X, y, d-1, r0, r1, _L, _R, _B, _T);
        if (X < w && Y < h) P[3] = new tile(name, w, h, s, X, Y, d-1, r0, r1, _L, _R, _B, _T);

        // Accumulate the AABBs of the children. */

        b[0] =  std::numeric_limits<double>::max();
        b[1] =  std::numeric_limits<double>::max();
        b[2] =  std::numeric_limits<double>::max();
        b[3] = -std::numeric_limits<double>::max();
        b[4] = -std::numeric_limits<double>::max();
        b[5] = -std::numeric_limits<double>::max();

        for (int k = 0; k < 4; ++k)
            if (P[k])
            {
                b[0] = std::min(b[0], P[k]->b[0]);
                b[1] = std::min(b[1], P[k]->b[1]);
                b[2] = std::min(b[2], P[k]->b[2]);
                b[3] = std::max(b[3], P[k]->b[3]);
                b[4] = std::max(b[4], P[k]->b[4]);
                b[5] = std::max(b[5], P[k]->b[5]);
            }
    }
    else
    {
        // Compute the AABB of this tile.

        double v[8][3];

        // TODO: watch out for R > pi and B < -pi/2

        sphere_to_vector(v[0], L, B, r0);
        sphere_to_vector(v[1], R, B, r0);
        sphere_to_vector(v[2], L, T, r0);
        sphere_to_vector(v[3], R, T, r0);
        sphere_to_vector(v[4], L, B, r1);
        sphere_to_vector(v[5], R, B, r1);
        sphere_to_vector(v[6], L, T, r1);
        sphere_to_vector(v[7], R, T, r1);

        b[0] = min8(v[0][0], v[1][0], v[2][0], v[3][0],
                    v[4][0], v[5][0], v[6][0], v[7][0]);
        b[1] = min8(v[0][1], v[1][1], v[2][1], v[3][1],
                    v[4][1], v[5][1], v[6][1], v[7][1]);
        b[2] = min8(v[0][2], v[1][2], v[2][2], v[3][2],
                    v[4][2], v[5][2], v[6][2], v[7][2]);
        b[3] = max8(v[0][0], v[1][0], v[2][0], v[3][0],
                    v[4][0], v[5][0], v[6][0], v[7][0]);
        b[4] = max8(v[0][1], v[1][1], v[2][1], v[3][1],
                    v[4][1], v[5][1], v[6][1], v[7][1]);
        b[5] = max8(v[0][2], v[1][2], v[2][2], v[3][2],
                    v[4][2], v[5][2], v[6][2], v[7][2]);
    }

    // Test loads

    if (d == 0 && i == 0x0c && j == 0x0d) object = load_png(name, d, i, j);
/*
    if (d == 0 && i == 0x0c && j == 0x0e) object = load_png(name, d, i, j);
*/
    if (d == 1 && i == 0x06 && j == 0x07) object = load_png(name, d, i, j);

    if (d == 7 && i == 0x00 && j == 0x00) object = load_png(name, d, i, j);
}

uni::tile::~tile()
{
    if (P[3]) delete P[3];
    if (P[2]) delete P[2];
    if (P[1]) delete P[1];
    if (P[0]) delete P[0];

    eject();
}

void uni::tile::ready(GLuint o)
{
    object = o;
    state  = live_state;
}

void uni::tile::eject()
{
    if (object)
        glDeleteTextures(1, &object);

    object = 0;
    state  = dead_state;
}

//-----------------------------------------------------------------------------

bool uni::tile::visible(const double *V,
                        const double *M,
                        const double *I)
{
    return true;
}

void uni::tile::draw(const double *V, const double *M, const double *I)
{
    if (visible(V, M, I))
    {
        if (object)
        {
            // Set up this tile's texture transform.

            double kt = +1.0 / (R - L);
            double kp = +1.0 / (T - B);
            double dt = -L * kt;
            double dp = -B * kp;

            glActiveTextureARB(GL_TEXTURE1);
            {
                glMatrixMode(GL_TEXTURE);
                {
                    glLoadIdentity();
                    glTranslated(dt, dp, 0.0);
                    glScaled    (kt, kp, 1.0);
                }
                glMatrixMode(GL_MODELVIEW);

                glBindTexture(GL_TEXTURE_2D, object);
            }
            glActiveTextureARB(GL_TEXTURE0);

            // Draw this tile's bounding volume.

            glBegin(GL_QUADS);
            {
                // -X
                glVertex3d(b[0], b[1], b[2]);
                glVertex3d(b[0], b[1], b[5]);
                glVertex3d(b[0], b[4], b[5]);
                glVertex3d(b[0], b[4], b[2]);

                // +X
                glVertex3d(b[3], b[1], b[5]);
                glVertex3d(b[3], b[1], b[2]);
                glVertex3d(b[3], b[4], b[2]);
                glVertex3d(b[3], b[4], b[5]);

                // -Y
                glVertex3d(b[0], b[1], b[2]);
                glVertex3d(b[3], b[1], b[2]);
                glVertex3d(b[3], b[1], b[5]);
                glVertex3d(b[0], b[1], b[5]);

                // +Y
                glVertex3d(b[0], b[4], b[5]);
                glVertex3d(b[3], b[4], b[5]);
                glVertex3d(b[3], b[4], b[2]);
                glVertex3d(b[0], b[4], b[2]);

                // -Z
                glVertex3d(b[3], b[1], b[2]);
                glVertex3d(b[0], b[1], b[2]);
                glVertex3d(b[0], b[4], b[2]);
                glVertex3d(b[3], b[4], b[2]);

                // +Z
                glVertex3d(b[0], b[1], b[5]);
                glVertex3d(b[3], b[1], b[5]);
                glVertex3d(b[3], b[4], b[5]);
                glVertex3d(b[0], b[4], b[5]);
            }
            glEnd();

            glActiveTextureARB(GL_TEXTURE1);
            {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
            glActiveTextureARB(GL_TEXTURE0);
        }

        if (P[0]) P[0]->draw(V, M, I);
        if (P[1]) P[1]->draw(V, M, I);
        if (P[2]) P[2]->draw(V, M, I);
        if (P[3]) P[3]->draw(V, M, I);
    }
}

void uni::tile::wire()
{
    if (object)
    {
        glBegin(GL_QUADS);
        {
            // -X
            glVertex3d(b[0], b[1], b[2]);
            glVertex3d(b[0], b[1], b[5]);
            glVertex3d(b[0], b[4], b[5]);
            glVertex3d(b[0], b[4], b[2]);

            // +X
            glVertex3d(b[3], b[1], b[5]);
            glVertex3d(b[3], b[1], b[2]);
            glVertex3d(b[3], b[4], b[2]);
            glVertex3d(b[3], b[4], b[5]);

            // -Y
            glVertex3d(b[0], b[1], b[2]);
            glVertex3d(b[3], b[1], b[2]);
            glVertex3d(b[3], b[1], b[5]);
            glVertex3d(b[0], b[1], b[5]);

            // +Y
            glVertex3d(b[0], b[4], b[5]);
            glVertex3d(b[3], b[4], b[5]);
            glVertex3d(b[3], b[4], b[2]);
            glVertex3d(b[0], b[4], b[2]);

            // -Z
            glVertex3d(b[3], b[1], b[2]);
            glVertex3d(b[0], b[1], b[2]);
            glVertex3d(b[0], b[4], b[2]);
            glVertex3d(b[3], b[4], b[2]);

            // +Z
            glVertex3d(b[0], b[1], b[5]);
            glVertex3d(b[3], b[1], b[5]);
            glVertex3d(b[3], b[4], b[5]);
            glVertex3d(b[0], b[4], b[5]);
        }
        glEnd();
    }

    if (P[0]) P[0]->wire();
    if (P[1]) P[1]->wire();
    if (P[2]) P[2]->wire();
    if (P[3]) P[3]->wire();
}

//-----------------------------------------------------------------------------

uni::geomap::geomap(std::string name,
                    int w, int h, int c, int b, int s,
                    double r0, double r1,
                    double L, double R, double B, double T) : name(name)
{
    // Compute the depth of the mipmap pyramid.

    int S = s;
    int d = 0;

    while (S < w || S < h)
    {
        S *= 2;
        d += 1;
    }

    // Generate the mipmap pyramid catalog.

    P = new tile(name, w, h, s, 0, 0, d, r0, r1, L, R, B, T);
}

uni::geomap::~geomap()
{
    if (P) delete P;
}

void uni::geomap::draw(const double *V, const double *M, const double *I)
{
    if (P) P->draw(V, M, I);
}

void uni::geomap::wire()
{
    if (P) P->wire();
}

//-----------------------------------------------------------------------------
