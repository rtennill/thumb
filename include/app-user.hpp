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

#include <app-file.hpp>

//-----------------------------------------------------------------------------

namespace app
{
    class user
    {
    private:

        double current_M[16];
        double current_I[16];
        double current_S[16];
        double current_L[3];
        double current_t;

        void set(const double *, const double *, double);

    public:

        user();

        void get_point(double *, const double *,
                       double *, const double *) const;

        const double  *get_M() const { return current_M; }
        const double  *get_I() const { return current_I; }
        const double  *get_S() const { return current_S; }
        const double  *get_L() const { return current_L; }
        double         get_t() const { return current_t; }

        void set_t(double);

        // Interactive view controls.

        void turn(double, double, double, const double *);
        void turn(double, double, double);
        void move(double, double, double);
        void look(double, double);
        void pass(double);
        void home();

        void set_M(const double *);
        void tumble(const double *,
                    const double *);

        // Transform application.

        void draw() const;
    };
}

//-----------------------------------------------------------------------------

extern app::user *user;

//-----------------------------------------------------------------------------

#endif
