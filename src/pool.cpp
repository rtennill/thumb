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

#include "matrix.hpp"
#include "glob.hpp"
#include "pool.hpp"

//=============================================================================

ogl::unit::unit(std::string name) :
    vc(0), ec(0),
    rebuff(true),
    my_node(0),
    surf(glob->load_surface(name))
{
    // Create a cache mesh for each mesh of the named surface.

    for (GLsizei i = 0; i < surf->max_mesh(); ++i)
        my_mesh[new mesh] = surf->get_mesh(i);

    // Total the vertex and element counts.

    for (mesh_m::iterator i = my_mesh.begin(); i != my_mesh.end(); ++i)
    {
        vc += i->second->count_verts();
        ec += i->second->count_lines() * 2
            + i->second->count_faces() * 3;
    }
}

ogl::unit::~unit()
{
    // Delete all cache meshes.

    for (mesh_m::iterator i = my_mesh.begin(); i != my_mesh.end(); ++i)
        delete i->first;

    glob->free_surface(surf);
}

void ogl::unit::set_node(node_p p)
{
    my_node = p;
}

//-----------------------------------------------------------------------------

void ogl::unit::transform(const GLfloat *M)
{
    load_mat(this->M, M);

    // Mark this unit and its node for a buffer update.

    if (my_node) my_node->set_rebuff();
    rebuff = true;
}

//-----------------------------------------------------------------------------

void ogl::unit::merge_batch(mesh_m& meshes)
{
    // Merge local meshes with the given set.  Meshes are sorted by material.

    meshes.insert(my_mesh.begin(), my_mesh.end());
}

void ogl::unit::merge_bound(aabb& b)
{
    // Merge the local mesh bounding volume with the given one.

    b.merge(my_aabb);
}

//-----------------------------------------------------------------------------

void ogl::unit::buff(bool b, GLfloat *v, GLfloat *n, GLfloat *t, GLfloat *u)
{
    if (b || rebuff)
    {
        my_aabb = aabb();

        // Transform and cache each mesh.  Accumulate bounding volumes.

        for (mesh_m::iterator i = my_mesh.begin(); i != my_mesh.end(); ++i)
        {
            i->first->cache_verts(i->second, M);
            i->first->merge_bound(my_aabb);
        }

        // Upload each mesh's vertex data to the bound buffer object.

        for (mesh_m::iterator i = my_mesh.begin(); i != my_mesh.end(); ++i)
        {
            const GLsizei ii = i->first->count_verts();

            i->first->buffv(v, n, t, u);

            v += ii * 3;
            n += ii * 3;
            t += ii * 3;
            u += ii * 2;
        }
    }
    rebuff = false;
}

//=============================================================================

ogl::node::node() :
    vc(0), ec(0),
    rebuff(true),
    my_pool(0)
{
    load_idt(M);
}

ogl::node::~node()
{
    for (unit_s::iterator i = my_unit.begin(); i != my_unit.end(); ++i)
        delete (*i);
}

//-----------------------------------------------------------------------------

void ogl::node::set_rebuff()
{
    if (my_pool) my_pool->set_rebuff();
    rebuff = true;
}

//-----------------------------------------------------------------------------

void ogl::node::set_pool(pool_p p)
{
    my_pool = p;
}

void ogl::node::add_unit(unit_p p)
{
    // Insert the given unit into the unit set.

    my_unit.insert(p);
    p->set_node(this);

    // Include the unit's vertex and element counts.

    vc += p->vcount();
    ec += p->ecount();

    if (my_pool) my_pool->add_vcount(+p->vcount());
    if (my_pool) my_pool->add_ecount(+p->ecount());

    // Mark this node's pool for a resort.

    if (my_pool) my_pool->set_resort();
}

void ogl::node::rem_unit(unit_p p)
{
    // Erase the given unit from the unit set.

    my_unit.erase(p);
    p->set_node(0);

    // Omit the unit's vertex and element counts.

    vc -= p->vcount();
    ec -= p->ecount();

    if (my_pool) my_pool->add_vcount(-p->vcount());
    if (my_pool) my_pool->add_ecount(-p->ecount());

    // Mark this node's pool for a resort.

    if (my_pool) my_pool->set_resort();
}

//-----------------------------------------------------------------------------

void ogl::node::buff(bool b, GLfloat *v, GLfloat *n, GLfloat *t, GLfloat *u)
{
    // Have each unit upload its vertex data to the bound buffer objects.

    if (b || rebuff)
    {
        for (unit_s::iterator i = my_unit.begin(); i != my_unit.end(); ++i)
        {
            const GLsizei vc = (*i)->vcount();

            (*i)->buff(b, v, n, t, u);

            v += vc * 3;
            n += vc * 3;
            t += vc * 3;
            u += vc * 2;
        }
    }
    rebuff = false;
}

void ogl::node::sort(GLuint *e, GLuint d)
{
    mesh_m my_mesh;
    mesh_v my_elem;

    // Create a list of all meshes of this node, sorted by material.

    for (unit_s::iterator i = my_unit.begin(); i != my_unit.end(); ++i)
        (*i)->merge_batch(my_mesh);

    for (mesh_m::iterator i = my_mesh.begin(); i != my_mesh.end(); ++i)
    {
        const GLsizei dc = i->first->count_verts();
        const GLsizei fc = i->first->count_faces();
        const GLsizei lc = i->first->count_lines();

        // Cache each offset mesh.

        i->first->cache_lines(i->second, d);
        i->first->cache_faces(i->second, d);

        // Upload elements to the bound buffer object.

        i->first->buffe(e);

        // Create a batch for each set of primatives.

        if (lc) my_elem.push_back(i->second->state(), e, GL_LINES,     lc * 2,
                                  i->first->get_min(),
                                  i->first->get_max())
        if (fc) my_elem.push_back(i->second->state(), e, GL_TRIANGLES, fc * 3,
                                  i->first->get_min(),
                                  i->first->get_max())

        // Iterate down the element buffer object.

        d += dc;
        e += lc * 2 + fc * 3;
    }

    // Create a minimal vector of batches for each draw mode.

    opaque_depth.clear();
    opaque_color.clear();
    transp_depth.clear();
    transp_color.clear();

    for (elem_v::iterator i = my_elem.begin(); i != my_elem.end(); ++i)
    {
        if (i->opaque())
        {
            // Opaque depth batches

            if (opaque_depth.empty() || !opaque_depth.back().depth_eq(*i))
                opaque_depth.push_back(*i);
            else
                opaque_depth.back().merge(*i);

            // Opaque color batches

            if (opaque_color.empty() || !opaque_color.back().color_eq(*i))
                opaque_color.push_back(*i);
            else
                opaque_color.back().merge(*i);
        }
        else
        {
            // Transp depth batches

            if (transp_depth.empty() || !transp_depth.back().depth_eq(*i))
                transp_depth.push_back(*i);
            else
                transp_depth.back().merge(*i);

            // Transp color batches

            if (transp_color.empty() || !transp_color.back().color_eq(*i))
                transp_color.push_back(*i);
            else
                transp_color.back().merge(*i);
        }
    }
}

//-----------------------------------------------------------------------------

void ogl::node::draw(bool color, bool alpha)
{
    // Test the bounding volume.

    // Render the selected batches.
}

//=============================================================================

ogl::pool::pool() : vc(0), ec(0), resort(true), rebuff(true), vbo(0), ebo(0)
{
    glGenBuffersARB(1, &vbo);
    glGenBuffersARB(1, &ebo);
}

ogl::pool::~pool()
{
    if (ebo) glDeleteBuffersARB(1, &ebo);
    if (vbo) glDeleteBuffersARB(1, &vbo);
}

//-----------------------------------------------------------------------------

void ogl::pool::set_resort()
{
    resort = true;
}

void ogl::pool::set_rebuff()
{
    rebuff = true;
}

void ogl::pool::add_vcount(GLsizei vc)
{
    this->vc += vc;
}

void ogl::pool::add_ecount(GLsizei ec)
{
    this->ec += ec;
}

//-----------------------------------------------------------------------------

void ogl::pool::add_node(node_p p)
{
    // Insert the given node into the node set.

    my_node.insert(p);
    p->set_pool(this);

    // Include the node's vertex and element counts.

    vc += p->vcount();
    ec += p->ecount();

    // Mark this pool and its pool for a resort.

    set_resort();
}

void ogl::pool::rem_node(node_p p)
{
    // Erase the given node from the node set.

    my_node.erase(p);
    p->set_pool(0);

    // Omit the node's vertex and element counts.

    vc -= p->vcount();
    ec -= p->ecount();

    // Mark this pool and its pool for a resort.

    set_resort();
}

//-----------------------------------------------------------------------------

void ogl::pool::buff(bool force)
{
    // Compute buffer object offsets for each vertex attribute.

    GLfloat *v = (GLfloat *) (0);
    GLfloat *n = (GLfloat *) (vc * sizeof (GLfloat) * 3);
    GLfloat *t = (GLfloat *) (vc * sizeof (GLfloat) * 6);
    GLfloat *u = (GLfloat *) (vc * sizeof (GLfloat) * 9);

    // Rebuff all nodes.

    for (node_s::iterator i = my_node.begin(); i != my_node.end(); ++i)
    {
        const GLsizei vc = (*i)->vcount();

        (*i)->buff(force, v, n, t, u);

        v += vc * 3;
        n += vc * 3;
        t += vc * 3;
        u += vc * 2;
    }
    rebuff = false;
}


void ogl::pool::sort()
{
    GLsizei vsz = vc * sizeof (GLfloat) * 11;
    GLsizei esz = ec * sizeof (GLuint);

    // Initialize vertex and element buffer sizes.

    glBufferDataARB(GL_ARRAY_BUFFER_ARB,         vsz, 0, GL_STATIC_DRAW_ARB);
    glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB, esz, 0, GL_STATIC_DRAW_ARB);

    // Resort all nodes.

    GLuint *e = 0;
    GLuint  d = 0;

    for (node_s::iterator i = my_node.begin(); i != my_node.end(); ++i)
    {
        (*i)->sort(e, d);

        e += (*i)->ecount();
        d += (*i)->vcount();
    }
    resort = false;

    // Force-rebuff all nodes.

    buff(true);
}

//-----------------------------------------------------------------------------

void ogl::pool::bind() const
{
    // Bind the VBO and EBO.

    glBindBufferARB(GL_ARRAY_BUFFER_ARB,         vbo);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, ebo);
}

void ogl::pool::free() const
{
    // Unbind the VBO and EBO.

    glBindBufferARB(GL_ARRAY_BUFFER_ARB,         0);
    glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
}

//-----------------------------------------------------------------------------

void ogl::pool::draw_init()
{
    bind();

    // Update the VBO and EBO as necessary.

    if (resort) sort(    );
    if (rebuff) buff(true);

    // Enable and attach the vertex arrays.

    glEnableVertexAttribArrayARB(6);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);

    GLfloat *v = (GLfloat *) (0);
    GLfloat *n = (GLfloat *) (vc * sizeof (GLfloat) * 3);
    GLfloat *t = (GLfloat *) (vc * sizeof (GLfloat) * 6);
    GLfloat *u = (GLfloat *) (vc * sizeof (GLfloat) * 9);

    glTexCoordPointer       (   2, GL_FLOAT,    0, u);
    glVertexAttribPointerARB(6, 3, GL_FLOAT, 0, 0, t);
    glNormalPointer         (      GL_FLOAT,    0, n);
    glVertexPointer         (   3, GL_FLOAT,    0, v);
}

void ogl::pool::draw_fini()
{
    // Disable the vertex arrays.

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableVertexAttribArrayARB(6);

    free();
}

//-----------------------------------------------------------------------------

void ogl::pool::draw(bool color, bool alpha)
{
    for (node_s::iterator i = my_node.begin(); i != my_node.end(); ++i)
        (*i)->draw(color, alpha);
}

//=============================================================================
