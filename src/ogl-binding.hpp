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

#ifndef OGL_BINDING_HPP
#define OGL_BINDING_HPP

#include <vector>
#include <string>
#include <map>

#include "app-serial.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    class frame;
    class program;
    class texture;
}

//-----------------------------------------------------------------------------

namespace ogl
{
    typedef std::map<GLenum, const ogl::texture *> unit_map;

    class binding
    {
        static std::vector<ogl::frame *> shadow;

        static GLfloat split[4];

        std::string name;

        const ogl::program *depth_program;
        const ogl::program *color_program;

        unit_map depth_texture;
        unit_map color_texture;

        static bool init_shadow();

    public:

        static int  shadow_count();

        static bool bind_shadow_frame(int);
        static bool bind_shadow_color(int, GLenum=GL_TEXTURE0);
        static bool bind_shadow_depth(int, GLenum=GL_TEXTURE0);

        static void draw_shadow_color(int);

        static void free_shadow_frame(int);
        static void free_shadow_color(int);
        static void free_shadow_depth(int);

        const std::string& get_name() const { return name; }

        binding(std::string);
       ~binding();

        static void set_split(GLfloat, GLfloat, GLfloat, GLfloat);

        bool opaque() const { return true; }

        bool depth_eq(const binding *) const;
        bool color_eq(const binding *) const;

        void bind(bool) const;

        void init();
        void fini();
    };
}

//-----------------------------------------------------------------------------

#endif
