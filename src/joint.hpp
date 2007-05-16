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

#ifndef JOINT_HPP
#define JOINT_HPP

#include "param.hpp"
#include "atom.hpp"

//-----------------------------------------------------------------------------

namespace wrl
{
    class joint : public atom
    {
    protected:

        int join_id;

    public:

        joint(dSpaceID, const ogl::surface *,
                        const ogl::surface *);

        virtual int join(int id) { return (join_id = id); }
        virtual int join() const { return (join_id     ); }

//      virtual void step_init();
        virtual void draw_line() const;

        virtual void         load(mxml_node_t *);
        virtual mxml_node_t *save(mxml_node_t *);

        virtual ~joint();
    };

    //-------------------------------------------------------------------------
    // Ball joint

    class ball : public joint
    {
    public:

        ball(dSpaceID);

        ball *clone() const { return new ball(*this); }

        mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Hinge joint

    class hinge : public joint
    {
    public:

        hinge(dSpaceID);

        hinge *clone() const { return new hinge(*this); }

//      void step_init();

        mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Suspension hinge joint

    class hinge2 : public joint
    {
    public:
        hinge2(dSpaceID);

        hinge2 *clone() const { return new hinge2(*this); }

//      void step_init();

        mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Prismatic slider joint

    class slider : public joint
    {
    public:
        slider(dSpaceID);

        slider *clone() const { return new slider(*this); }

//      void step_init();

        mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Angular motor joint

    class amotor : public joint
    {
    public:
        amotor(dSpaceID);

        amotor *clone() const { return new amotor(*this); }

//      void step_init();

        mxml_node_t *save(mxml_node_t *);
    };

    //-------------------------------------------------------------------------
    // Universal joint

    class universal : public joint
    {
    public:
        universal(dSpaceID);

        universal *clone() const { return new universal(*this); }

//      void step_init();

        mxml_node_t *save(mxml_node_t *);
    };
}

//-----------------------------------------------------------------------------

#endif
