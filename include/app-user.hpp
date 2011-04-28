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

#ifndef USER_HPP
#define USER_HPP

#include "app-file.hpp"

//-----------------------------------------------------------------------------

namespace app
{
    class user
    {
    private:

        // View matrix and inverse cache

        double cache_M[16];
        double cache_I[16];

        double current_M[16];
        double current_I[16];
        double current_S[16];
        double current_L[3];
        double current_t;

        double move_rate;
        double turn_rate;
        double move_rate_k;
        double turn_rate_k;

        void set(const double *, const double *, double);

        // A bit of a hack: fly-to state.

        double fly_r_min;
        double fly_r_max;

        bool   auto_b;
        double auto_t;
        double auto_T;
        double auto_n0[3];
        double auto_n1[3];
        double auto_q0[4];
        double auto_q1[4];
        double auto_r0;
        double auto_r1;

        // Automatic demo file.

        app::file file;

        app::node root;
        app::node prev;
        app::node curr;
        app::node next;
        app::node pred;

        app::node cycle_next(app::node);
        app::node cycle_prev(app::node);

        void correct();

        double interpolate(app::node, app::node, const char *, double);

        void erp_state(app::node, app::node, double, int&);
        void set_state(app::node,                    int&);

        double tt;

        bool stopped;

    public:

        user();

        void get_point(double *, const double *,
                       double *, const double *) const;

        const double  *get_M() const { return current_M; }
        const double  *get_I() const { return current_I; }
        const double  *get_S() const { return current_S; }
        const double  *get_L() const { return current_L; }
        double         get_t() const { return current_t; }
        double get_move_rate() const { return move_rate_k; }
        double get_turn_rate() const { return turn_rate_k; }

        void put_move_rate(double r) { move_rate = r; }
        void put_turn_rate(double r) { turn_rate = r; }

        void set_t(double);

        // Interactive view controls.

        void turn(double, double, double, const double *);
        void turn(double, double, double);
        void move(double, double, double);
        void look(double, double);
        void pass(double);
        void home();

        void fly(double, double, double, double=0.0);

        void tumble(const double *,
                    const double *);

        // Autopilot

        void auto_init(const double *);
        void auto_step(double);
        void auto_stop();

        // Automatic view controls.

        bool dostep(double, int&);
        void gonext();
        void goprev();
        void gohalf();
        void insert(int);
        void remove();

        // Transform application.

        void draw() const;
    };
}

//-----------------------------------------------------------------------------

extern app::user *user;

//-----------------------------------------------------------------------------

#endif