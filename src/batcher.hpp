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

#ifndef BATCHER_HPP
#define BATCHER_HPP

#include <map>
#include <set>

#include "binding.hpp"
#include "surface.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    //-------------------------------------------------------------------------
    // TODO: implement mesh sorting based on binding order
    // TODO: implement binding sorting

    class element;
    class segment;
    class batcher;

    typedef std::set<element *>           element_s;
    typedef std::set<element *>::iterator element_i;
    typedef std::set<segment *>           segment_s;
    typedef std::set<segment *>::iterator segment_i;

    typedef std::map<const mesh *, vert *>                 mesh_vert_m;
    typedef std::map<const mesh *, vert *>::iterator       mesh_vert_i;
    typedef std::map<const mesh *, vert *>::const_iterator mesh_vert_c;

    typedef std::multimap<const mesh *, element *>                 mesh_elem_m;
    typedef std::multimap<const mesh *, element *>::iterator       mesh_elem_i;
    typedef std::multimap<const mesh *, element *>::const_iterator mesh_elem_c;

    //-------------------------------------------------------------------------

    struct batch
    {
        const binding *bnd;
        const GLuint  *off;

        GLenum  typ;
        GLsizei num;
        GLuint  min;
        GLuint  max;

        batch(const binding *b,
              GLuint *o, GLenum t, GLsizei n, GLuint a, GLuint z) :
            bnd(b), off(o), typ(t), num(n), min(a), max(z) { }
    };

    typedef std::vector<batch>                 batch_v;
    typedef std::vector<batch>::iterator       batch_i;
    typedef std::vector<batch>::const_iterator batch_c;

    //-------------------------------------------------------------------------
    // Batchable element

    class element
    {
        bool status;

        const surface *srf;
        mesh_vert_m    off;

        GLfloat M[16];
        GLfloat I[16];

    public:

        element(const element&);
        element(std::string);
       ~element();

        void move(const GLfloat *);

        void live() { status = true;  }
        void dead() { status = false; }

        void box_bound(GLfloat *b) const { return srf->box_bound(b); }
        void sph_bound(GLfloat *b) const { return srf->sph_bound(b); }

        // Batch data handlers

        GLsizei vcount() const;
        GLsizei ecount() const;

        void enlist(mesh_elem_m&,
                    mesh_elem_m&);

        vert *vert_cache(const mesh *, vert *);
    };

    //-------------------------------------------------------------------------
    // Batch segment

    class segment
    {
        element_s elements;

        batch_v opaque_bat;
        batch_v transp_bat;

        GLfloat M[16];

    public:

        segment();
       ~segment();

        void move(const GLfloat *);

        // Element handlers

        void insert(element *e) { if (e) { elements.insert(e); e->live(); }}
        void remove(element *e) { if (e) { elements.erase (e); e->dead(); }}

        void clear();

        // Batch data handlers

        GLsizei vcount() const;
        GLsizei ecount() const;

        GLuint *reduce(vert_p&, GLuint *, mesh_elem_m&, batch_v&);
        GLuint *enlist(vert_p&, GLuint *);

        // Renderers

        void draw_opaque(bool) const;
        void draw_transp(bool) const;
    };

    //-------------------------------------------------------------------------
    // Batch manager

    class batcher
    {
        segment_s segments;

        GLuint vbo;
        GLuint ebo;

        bool status;

    public:
        
        batcher();
       ~batcher();

        void dirty();
        void clean();
        void clear();

        // Segment handlers

        void insert(segment *s) { if (s) { segments.insert(s); dirty(); }}
        void remove(segment *s) { if (s) { segments.erase (s); dirty(); }}

        // Renderers

        void bind() const;
        void free() const;

        void draw_init();
        void draw_fini();

        void draw_opaque(bool) const;
        void draw_transp(bool) const;
    };
}

//-----------------------------------------------------------------------------

#endif
