//  Copyright (C) 2007-2011 Robert Kooima
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

#ifndef DPY_CHANNEL_HPP
#define DPY_CHANNEL_HPP

#include <vector>

#include <etc-vector.hpp>
#include <app-default.hpp>
#include <app-file.hpp>

//-----------------------------------------------------------------------------

namespace ogl
{
    class frame;
    class program;
}

namespace app
{
    class event;
}

//-----------------------------------------------------------------------------

namespace dpy
{
    class channel
    {
    public:

        channel(app::node, int[2]);
       ~channel();

        // Accessors

        void set_head(const vec3&, const quat&);

        vec3   get_eye()    const { return p; }
        int    get_width()  const { return w; }
        int    get_height() const { return h; }
        GLuint get_color()  const;
        GLuint get_depth()  const;

        // Rendering methods

        void test() const;
        void bind(double=1.0) const;
        void free() const;
        void proc() const;

        void bind_color(GLenum t) const;
        void free_color(GLenum t) const;

        // Event handler

        bool process_event(app::event *);

    private:

        static const ogl::program *downsample_avg;
        static const ogl::program *downsample_max;
        static const ogl::program *h_gaussian;
        static const ogl::program *v_gaussian;
        static const ogl::program *tonemap;
        static const ogl::program *bloom;

        static ogl::frame *blur;  // Bloom buffer
        static ogl::frame *ping;  // Process ping-pong buffer
        static ogl::frame *pong;  // Process ping-pong buffer

        ogl::frame *src;          // Off-screen render target
        ogl::frame *dst;          // Off-screen render target
        int w;                    // Off-screen render target width
        int h;                    // Off-screen render target height

        vec3 v;                   // Eye position, in head coordinates
        vec3 p;                   // Eye position, in user coordinates (cache)

        GLubyte c[4];             // Calibration target color

        void process_start();
        void process_close();
    };

    typedef std::vector<channel *>           channel_v;
    typedef std::vector<channel *>::iterator channel_i;
}

//-----------------------------------------------------------------------------

#endif
