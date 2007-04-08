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

#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <string>

#include "opengl.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    class program
    {
        std::string name;

        GLhandleARB vert;
        GLhandleARB frag;
        GLhandleARB prog;

        void log(GLhandleARB);

    public:

        const std::string& get_name() const { return name; }

        program(std::string);
       ~program();

        void bind() const;
        void free() const;

        void uniform(std::string, int)                        const;
        void uniform(std::string, float)                      const;
        void uniform(std::string, float, float)               const;
        void uniform(std::string, float, float, float)        const;
        void uniform(std::string, float, float, float, float) const;
    };
}

#endif