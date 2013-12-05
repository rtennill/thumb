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

#ifndef USER_HPP
#define USER_HPP

#include <etc-vector.hpp>

//-----------------------------------------------------------------------------

namespace app
{
    class view
    {
    private:

        quat orientation;
        vec3 position;
        mat4 tracking;
        bool vertical;

    public:

        view();

        void go_home();


        vec3 get_point_pos(const vec3&) const;
        vec3 get_point_vec(const quat&) const;

        void set_orientation(const quat& q);
        void set_position   (const vec3& p) { position    = p; }
        void set_tracking   (const mat4& M) { tracking    = M; }
        void lock_vertical  (bool b)        { vertical    = b; }

        mat4 get_tracking   () const { return tracking;    }
        quat get_orientation() const { return orientation; }
        vec3 get_position   () const { return position;    }

        mat4  get_inverse  () const;
        mat4  get_transform() const;
        void load_transform() const;
    };
}

//-----------------------------------------------------------------------------

extern app::view *view;

//-----------------------------------------------------------------------------

#endif
