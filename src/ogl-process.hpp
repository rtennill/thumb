//  Copyright (C) 2009 Robert Kooima
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

#ifndef OGL_PROCESS_HPP
#define OGL_PROCESS_HPP

#include <string>

#include "ogl-opengl.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    class pool;
    class node;
    class frame;
    class binding;
}

//-----------------------------------------------------------------------------

namespace ogl
{
    class process
    {
    protected:

        static ogl::pool *cube_pool;
        static ogl::node *cube_node[6];

        static void init_cube();
        static void fini_cube();
        static void proc_cube(const ogl::binding *, ogl::frame *);

    public:

        virtual const std::string& get_name() const = 0;

        process();

        virtual void exec(const ogl::binding *) const { }
        virtual void bind(GLenum)               const { }

        virtual void init() { }
        virtual void fini() { }

        virtual ~process();
    };
}

//-----------------------------------------------------------------------------

#endif
