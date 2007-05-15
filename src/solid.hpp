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

#ifndef SOLID_HPP
#define SOLID_HPP

#include "param.hpp"
#include "atom.hpp"
#include "glob.hpp"

//-----------------------------------------------------------------------------

namespace wrl
{
    class solid : public atom
    {
    protected:

        virtual void scale() = 0;

    public:

        solid(const ogl::surface *,
              const ogl::surface *);

        virtual void step_post();

        virtual void         load(mxml_node_t *);
        virtual mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Solid box atom

    class box : public solid
    {
    protected:

        virtual void scale();

    public:

        box(dSpaceID, const ogl::surface *);

        virtual box *clone() const { return new box(*this); }

        virtual void play_init(dBodyID);
        virtual void draw_line() const;

        virtual mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Solid sphere atom

    class sphere : public solid
    {
    protected:

        virtual void scale();

    public:

        sphere(dSpaceID, const ogl::surface *);

        virtual sphere *clone() const { return new sphere(*this); }

        virtual void play_init(dBodyID);
        virtual void draw_line() const;

        virtual mxml_node_t *save(mxml_node_t *);
    };
}

//-----------------------------------------------------------------------------

#endif
