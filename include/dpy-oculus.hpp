//  Copyright (C) 2013 Robert Kooima
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

#ifdef CONFIG_OCULUS
#ifndef DPY_OCULUS_HPP
#define DPY_OCULUS_HPP

//-----------------------------------------------------------------------------

// OVR's definition of static_assert conflicts with LLVM's. IMO, LLVM has dibs,
// so this undef should eventually become unnecessary.

#undef static_assert

#include <OVR.h>
#include <OVR_CAPI_GL.h>

//-----------------------------------------------------------------------------

#include <dpy-display.hpp>
#include <app-file.hpp>

namespace ogl
{
    class program;
}

//-----------------------------------------------------------------------------

namespace dpy
{
    class oculus : public display
    {
    public:

        oculus(app::node, int[4], int[2]);

        virtual ~oculus();

        // Frustum queries

        virtual int  get_frusc()                const;
        virtual void get_frusv(app::frustum **) const;

        virtual app::frustum *get_overlay() const { return 0; } //frust[0]; }

        // Rendering handlers

        virtual void prep(int, const dpy::channel * const *);
        virtual void draw(int, const dpy::channel * const *, int);
        virtual void test(int, const dpy::channel * const *, int);

        // Event handers

        virtual bool pointer_to_3D(app::event *, int, int);
        virtual bool process_event(app::event *);

    private:

        ovrHmd          hmd;
        ovrVector3f  offset[2];
        ovrPosef       pose[2];
        ovrTexture      tex[2];
        app::frustum *frust[2];
        mat4     projection[2];

        virtual bool process_start(app::event *);
        virtual bool process_close(app::event *);

        void dismiss_warning();
    };
}

//-----------------------------------------------------------------------------

#endif
#endif
