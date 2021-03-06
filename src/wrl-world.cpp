//  Copyright (C) 2007-2011 Robert Kooima
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

#include <algorithm>
#include <iterator>
#include <iostream>
#include <cassert>

#include <etc-log.hpp>
#include <etc-vector.hpp>
#include <etc-ode.hpp>
#include <ogl-pool.hpp>
#include <ogl-uniform.hpp>
#include <ogl-process.hpp>
#include <app-glob.hpp>
#include <app-conf.hpp>
#include <app-view.hpp>
#include <app-file.hpp>
#include <app-frustum.hpp>
#include <wrl-solid.hpp>
#include <wrl-light.hpp>
#include <wrl-joint.hpp>
#include <wrl-world.hpp>

#define MAX_CONTACTS 64

//-----------------------------------------------------------------------------

wrl::world::world() :
    serial(1),
    shadow_splits(::conf->get_i("shadow_map_splits", 3))
{
    // Initialize the editor physical system.

    dInitODE();

    edit_space = dHashSpaceCreate(0);
    edit_point = dCreateRay(edit_space, 1000);
    edit_focus = 0;

    play_world = 0;
    play_scene = 0;
    play_actor = 0;
    play_joint = 0;

    // Initialize the render pools.

    fill_pool = ::glob->new_pool();
    fill_node = new ogl::node;

    line_pool = ::glob->new_pool();
    line_node = new ogl::node;

    fill_pool->add_node(fill_node);
    line_pool->add_node(line_node);

    // Initialize the render uniforms and processes.

    uniform_shadow[0] = ::glob->load_uniform("ShadowMatrix[0]",   16);
    uniform_shadow[1] = ::glob->load_uniform("ShadowMatrix[1]",   16);
    uniform_shadow[2] = ::glob->load_uniform("ShadowMatrix[2]",   16);
    uniform_shadow[3] = ::glob->load_uniform("ShadowMatrix[3]",   16);
    uniform_light[0]  = ::glob->load_uniform("LightPosition[0]",   4);
    uniform_light[1]  = ::glob->load_uniform("LightPosition[1]",   4);
    uniform_light[2]  = ::glob->load_uniform("LightPosition[2]",   4);
    uniform_light[3]  = ::glob->load_uniform("LightPosition[3]",   4);
    uniform_split[0]  = ::glob->load_uniform("LightSplit[0]",      2);
    uniform_split[1]  = ::glob->load_uniform("LightSplit[1]",      2);
    uniform_split[2]  = ::glob->load_uniform("LightSplit[2]",      2);
    uniform_split[3]  = ::glob->load_uniform("LightSplit[3]",      2);
    uniform_bright[0] = ::glob->load_uniform("LightBrightness[0]", 2);
    uniform_bright[1] = ::glob->load_uniform("LightBrightness[1]", 2);
    uniform_bright[2] = ::glob->load_uniform("LightBrightness[2]", 2);
    uniform_bright[3] = ::glob->load_uniform("LightBrightness[3]", 2);

    uniform_highlight = ::glob->load_uniform("Highlight",   1);
    uniform_spot      = ::glob->load_uniform("LightCutoff", 4);
    uniform_unit      = ::glob->load_uniform("LightUnit",   4);

    process_shadow[0] = ::glob->load_process("shadow", 0);
    process_shadow[1] = ::glob->load_process("shadow", 1);
    process_shadow[2] = ::glob->load_process("shadow", 2);
    process_shadow[3] = ::glob->load_process("shadow", 3);
    process_cookie[0] = ::glob->load_process("cookie", 0);
    process_cookie[1] = ::glob->load_process("cookie", 1);
    process_cookie[2] = ::glob->load_process("cookie", 2);
    process_cookie[3] = ::glob->load_process("cookie", 3);

//  click_selection(new wrl::box("solid/bunny.obj"));
//  click_selection(new wrl::box("solid/buddha.obj"));
//  do_create();
}

wrl::world::~world()
{
    play_fini();

    // Atoms own units, so units must be removed from nodes before deletion.

    fill_node->clear();
    line_node->clear();

    for (node_map::iterator j = nodes.begin(); j != nodes.end(); ++j)
        j->second->clear();

    // Delete all operations.

    while (!undo_list.empty())
    {
        delete undo_list.front();
        undo_list.pop_front();
    }
    while (!redo_list.empty())
    {
        delete redo_list.front();
        redo_list.pop_front();
    }

    // Finalize the atoms.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        delete (*i);

    // Finalize the editor physical system.

    dGeomDestroy (edit_point);
    dSpaceDestroy(edit_space);

    dCloseODE();

    // Finalize the uniforms and processes.

    for (int i = 0; i < 4; ++i)
    {
        ::glob->free_process(process_cookie[i]);
        ::glob->free_process(process_shadow[i]);

        ::glob->free_uniform(uniform_shadow[i]);
        ::glob->free_uniform(uniform_light [i]);
        ::glob->free_uniform(uniform_split [i]);
        ::glob->free_uniform(uniform_bright[i]);
    }

    ::glob->free_uniform(uniform_highlight);
    ::glob->free_uniform(uniform_spot);
    ::glob->free_uniform(uniform_unit);

    // Finalize the render pools.

    ::glob->free_pool(fill_pool);
    ::glob->free_pool(line_pool);
}

//-----------------------------------------------------------------------------

void wrl::world::edit_callback(dGeomID o1, dGeomID o2)
{
    dContact contact[MAX_CONTACTS];
    int sz = sizeof (dContact);

    dGeomID O1;
    dGeomID O2;

    if      (o1 == edit_point) { O1 = o1; O2 = o2; }
    else if (o2 == edit_point) { O1 = o2; O2 = o1; }
    else return;

    // Note the nearest picking ray collision with a placeable geom.

    if (dGeomGetClass(O2) != dPlaneClass)
    {
        if (int n = dCollide(O1, O2, MAX_CONTACTS, &contact[0].geom, sz))
        {
            for (int i = 0; i < n; ++i)
            {
                if (focus_dist > double(contact[i].geom.depth))
                {
                    focus_dist = double(contact[i].geom.depth);
                    edit_focus = O2;
                }
            }
        }
    }
}

void wrl::world::play_callback(dGeomID o1, dGeomID o2)
{
    dBodyID b1 = dGeomGetBody(o1);
    dBodyID b2 = dGeomGetBody(o2);

    // Ignore collisions between geoms associated with the same body.

    if (b1 != b2)
    {
        dContact contact[MAX_CONTACTS];
        int sz = sizeof (dContact);

        // Check for collisions between these two geoms.

        if (int n = dCollide(o1, o2, MAX_CONTACTS, &contact[0].geom, sz))
        {
            /* TODO: Reimplement collision triggers with cluster awareness.
            set_trg(dGeomGetCategoryBits(o1));
            set_trg(dGeomGetCategoryBits(o2));
            */

            atom *a1 = (atom *) dGeomGetData(o1);
            atom *a2 = (atom *) dGeomGetData(o2);

            // Apply the solid surface parameters.

            dSurfaceParameters surface;

            surface.mode = dContactBounce
                         | dContactSoftCFM
                         | dContactSoftERP;

            surface.mu         = dInfinity;
            surface.bounce     = 0.0;
            surface.bounce_vel = 0.1;
            surface.soft_erp   = 1.0;
            surface.soft_cfm   = 0.0;

            a1->get_surface(surface);
            a2->get_surface(surface);

            // Create a contact joint for each collision.

            for (int i = 0; i < n; ++i)
            {
                contact[i].surface = surface;
                dJointAttach(dJointCreateContact(play_world, play_joint,
                                                 contact + i), b1, b2);
            }
        }
    }
}

void edit_callback(wrl::world *that, dGeomID o1, dGeomID o2)
{
    that->edit_callback(o1, o2);
}

void play_callback(wrl::world *that, dGeomID o1, dGeomID o2)
{
    that->play_callback(o1, o2);
}

//-----------------------------------------------------------------------------

void wrl::world::play_init()
{
    int id;

    // Create the world, collision spaces, and joint group.

    play_world = dWorldCreate();
    play_scene = dHashSpaceCreate(0);
    play_actor = dHashSpaceCreate(0);
    play_joint = dJointGroupCreate(0);

    dWorldSetGravity        (play_world, 0, -9.8, 0);
    dWorldSetDamping        (play_world, 0.001, 0.001);
    dWorldSetAutoDisableFlag(play_world, 1);

    // Create a body and mass for each active entity group.

    mass_map play_mass;

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        if ((id = (*i)->body()))
        {
            if (play_body[id] == 0)
            {
                dBodyID body = dBodyCreate(play_world);
                dMass   mass;

                dMassSetZero(&mass);

                play_body[id] = body;
                play_mass[id] = mass;

                // Associate the render node.

                dBodySetData(body, nodes[id]);
            }
        }

    // Create a geom for each colliding atom.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
    {
        id = (*i)->body();

        dBodyID body = play_body[id];
        dGeomID geom;
        dMass   mass;

        // Add the geom to the correct space.

        if (body)
            geom = (*i)->init_play_geom(play_actor);
        else
            geom = (*i)->init_play_geom(play_scene);

        if (geom)
        {
            // Attach the geom to its body.

            dGeomSetBody(geom, body);

            // Accumulate the body mass.

            if (body)
            {
                (*i)->init_play_mass(&mass);
                dMassAdd(&play_mass[id], &mass);
            }
        }
    }

    // Position the bodies.

    for (body_map::iterator b = play_body.begin(); b != play_body.end(); ++b)
    {
        int     id   = b->first;
        dBodyID body = b->second;
        dMass   mass = play_mass[id];

        if (body)
        {
            // Center the body on its center of mass.

            dBodySetPosition(body, +mass.c[0], +mass.c[1], +mass.c[2]);
            dMassTranslate (&mass, -mass.c[0], -mass.c[1], -mass.c[2]);

            dBodySetMass(body, &mass);

            // Recenter the node on the body.

            mat4 M = translation(vec3(double(mass.c[0]),
                                      double(mass.c[1]),
                                      double(mass.c[2])));

            ((ogl::node *) dBodyGetData(body))->transform(M);
        }
    }

    // Create and attach all joints.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        if (dJointID join = (*i)->init_play_join(play_world))
            dJointAttach(join, play_body[(*i)->body()],
                               play_body[(*i)->join()]);

    // Do atom-specific physics initialization.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        (*i)->play_init();
}

void wrl::world::play_fini()
{
    // Reset all node transforms.

    for (node_map::iterator j = nodes.begin(); j != nodes.end(); ++j)
        j->second->transform(mat4());

    // Do atom-specific physics finalization.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        (*i)->play_fini();

    // Clean up the play-mode physics data.

    play_body.clear();

    if (play_joint) dJointGroupDestroy(play_joint);
    if (play_scene) dSpaceDestroy     (play_scene);
    if (play_actor) dSpaceDestroy     (play_actor);
    if (play_world) dWorldDestroy     (play_world);

    play_world = 0;
    play_scene = 0;
    play_actor = 0;
    play_joint = 0;
}

void wrl::world::play_step(double dt)
{
    // Do atom-specific physics step initialization.

    for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        (*i)->step_init();

    // Perform collision detection.

    // TODO: move clr_trg somewhere
    // clr_trg();

    dSpaceCollide2((dGeomID) play_actor, (dGeomID) play_scene,
                              this, (dNearCallback *) ::play_callback);
    dSpaceCollide(play_actor, this, (dNearCallback *) ::play_callback);

    // Evaluate the physical system.

    dWorldQuickStep (play_world, dt);
    dJointGroupEmpty(play_joint);

    // Transform all segments using current ODE state.

    for (body_map::iterator b = play_body.begin(); b != play_body.end(); ++b)
        if (dBodyID body = b->second)
            if (ogl::node *node = (ogl::node *) dBodyGetData(body))
                node->transform(bBodyGetTransform(body));
}

//-----------------------------------------------------------------------------

void wrl::world::edit_step(double dt)
{
    // Perform collision detection.

    focus_dist = 100;
    edit_focus =   0;
    dSpaceCollide(edit_space, this, (dNearCallback *) ::edit_callback);
}

void wrl::world::edit_pick(const vec3& p, const vec3& v)
{
    // These assertions head off a variety of difficult-to-track issues.

    assert(!std::isnan(p[0]));
    assert(!std::isnan(p[1]));
    assert(!std::isnan(p[2]));
    assert(!std::isnan(v[0]));
    assert(!std::isnan(v[1]));
    assert(!std::isnan(v[2]));

    // Apply the pointer position and vector to the picking ray.

    dGeomRaySet(edit_point, p[0], p[1], p[2], v[0], v[1], v[2]);
}

//-----------------------------------------------------------------------------

void wrl::world::doop(wrl::operation_p op)
{
    // Delete all redo-able operations.

    while (!redo_list.empty())
    {
        delete redo_list.front();
        redo_list.pop_front();
    }

    // Add this operation to the undo-able list.

    undo_list.push_front(op);

    // Do the operation for the first time.

    select_set(op->redo(this));
}

void wrl::world::undo()
{
    // Undo an operation and move it to the redo-able list.

    if (!undo_list.empty())
    {
        select_set(undo_list.front()->undo(this));

        redo_list.push_front(undo_list.front());
        undo_list.pop_front();
    }
}

void wrl::world::redo()
{
    // Redo an operation and move it to the undo-able list.

    if (!redo_list.empty())
    {
        select_set(redo_list.front()->redo(this));

        undo_list.push_front(redo_list.front());
        redo_list.pop_front();
    }
}

//-----------------------------------------------------------------------------

void wrl::world::set_param(int k, std::string& expr)
{
    for (atom_set::iterator i = sel.begin(); i != sel.end(); ++i)
        (*i)->set_param(k, expr);
}

int wrl::world::get_param(int k, std::string& expr)
{
    std::set<std::string> values;

    // Determine the number of distinct values among the selection's params.

    for (atom_set::iterator i = sel.begin(); i != sel.end(); ++i)
        if ((*i)->get_param(k, expr))
            values.insert(expr);

    return int(values.size());
}

//-----------------------------------------------------------------------------

void wrl::world::node_insert(int id, ogl::unit *fill, ogl::unit *line)
{
    if (id)
    {
        // Ensure that a node exits for this ID.

        if (nodes[id] == 0)
        {
            nodes[id] = new ogl::node();
            fill_pool->add_node(nodes[id]);
        }

        // Add the given unit to the node.

        nodes[id]->add_unit(fill);
    }
    else fill_node->add_unit(fill);

    line_node->add_unit(line);
}

void wrl::world::node_remove(int id, ogl::unit *fill, ogl::unit *line)
{
    if (id)
    {
        // Remove the unit from its current node.

        nodes[id]->rem_unit(fill);

        // If the node is empty then delete it.

        if (nodes[id]->vcount() == 0)
        {
            fill_pool->rem_node(nodes[id]);
            delete nodes[id];
            nodes.erase(id);
        }
    }
    else fill_node->rem_unit(fill);

    line_node->rem_unit(line);
}

//-----------------------------------------------------------------------------

void wrl::world::click_selection(atom *a)
{
    if (sel.find(a) != sel.end())
        sel.erase(a);
    else
        sel.insert(a);

    select_set(sel);
}

void wrl::world::clone_selection()
{
    atom_set clones;

    // Remove all selected entities.

    for (atom_set::iterator i = sel.begin(); i != sel.end(); ++i)
        clones.insert((*i)->clone());

    // Select the clones and push an undo-able create operation for them.

    select_set(clones);
    do_create();
}

void wrl::world::clear_selection()
{
    select_set();
}

bool wrl::world::check_selection()
{
    return !sel.empty();
}

//-----------------------------------------------------------------------------

void wrl::world::invert_selection()
{
    // Begin with the set of all entities.

    atom_set unselected;
    atom_set::iterator i;

    unselected = all;

    // Remove all selected entities.

    for (i = sel.begin(); i != sel.end(); ++i)
        unselected.erase(unselected.find(*i));

    // Giving the set of all unselected entities.

    select_set(unselected);
}

void wrl::world::extend_selection()
{
    std::set<int> ids;

    // Define a set of the body and join IDs of all selected entities.

    for (atom_set::iterator i = sel.begin(); i != sel.end(); ++i)
    {
        if ((*i)->body()) ids.insert((*i)->body());
        if ((*i)->join()) ids.insert((*i)->join());
    }

    // Add all entities with an included body or join ID to the selection.

    for (atom_set::iterator j = all.begin(); j != all.end(); ++j)
    {
        if (ids.find((*j)->body()) != ids.end()) sel.insert(*j);
    }

    select_set(sel);
}

//-----------------------------------------------------------------------------

void wrl::world::select_set()
{
    sel.clear();
}

void wrl::world::select_set(atom_set& set)
{
    sel = set;
}

//-----------------------------------------------------------------------------

void wrl::world::create_set(atom_set& set)
{
    for (atom_set::iterator i = set.begin(); i != set.end(); ++i)
    {
        // Add the atom's unit to the render pool.

        node_insert((*i)->body(), (*i)->get_fill(), (*i)->get_line());

        // Add the atom to the atom set.

        all.insert(*i);
        (*i)->live(edit_space);

        // Set the default transform.

        (*i)->transform(mat4());
    }
}

void wrl::world::delete_set(atom_set& set)
{
    for (atom_set::iterator i = set.begin(); i != set.end(); ++i)
    {
        // Remove the atom's unit from the render pool.

        node_remove((*i)->body(), (*i)->get_fill(), (*i)->get_line());

        // Remove the atom from the atom set.

        all.erase(all.find(*i));
        (*i)->dead(edit_space);
    }
}

void wrl::world::embody_set(atom_set& set, atom_map& map)
{
    for (atom_set::iterator i = set.begin(); i != set.end(); ++i)
    {
        // Assign the body ID for each atom.

        node_remove((*i)->body(), (*i)->get_fill(), (*i)->get_line());

        (*i)->body(map[*i]);

        node_insert((*i)->body(), (*i)->get_fill(), (*i)->get_line());
    }
}

void wrl::world::modify_set(atom_set& set, const mat4& T)
{
    // Apply the given transform to all given atoms.

    for (atom_set::iterator i = set.begin(); i != set.end(); ++i)
        (*i)->transform(T);
}

//-----------------------------------------------------------------------------

void wrl::world::do_create()
{
    // Ensure that the new entity body IDs do not conflict with existing ones.

    std::set<int> A;
    std::set<int> B;
    std::set<int> C;

    atom_set::iterator i;
    atom_set::iterator j;

    // Find all conflicting body IDs.

    for (i = all.begin(); i != all.end(); ++i)
        if ((*i)->body()) A.insert((*i)->body());

    for (j = sel.begin(); j != sel.end(); ++j)
        if ((*j)->body()) B.insert((*j)->body());

    std::set_intersection(A.begin(), A.end(),
                          B.begin(), B.end(), std::inserter(C, C.begin()));

    // Generate a new body ID for each conflicting ID.

    std::set<int>::iterator k;
    std::map<int, int>      M;

    for (k = C.begin(); k != C.end(); ++k)
        if (*k) M[*k] = serial++;

    // Remap conflicting IDs.

    for (j = sel.begin(); j != sel.end(); ++j)
    {
        if (M[(*j)->body()]) (*j)->body(M[(*j)->body()]);
        if (M[(*j)->join()]) (*j)->join(M[(*j)->join()]);
    }

    // (This will have nullified all broken joint target IDs.)

    if (!sel.empty()) doop(new wrl::create_op(sel));
}

void wrl::world::do_delete()
{
    if (!sel.empty()) doop(new wrl::delete_op(sel));
}

void wrl::world::do_enjoin()
{
    if (!sel.empty()) doop(new wrl::enjoin_op(sel));
}

void wrl::world::do_embody()
{
    if (!sel.empty()) doop(new wrl::embody_op(sel, serial++));
}

void wrl::world::do_debody()
{
    if (!sel.empty()) doop(new wrl::embody_op(sel, 0));
}

void wrl::world::do_modify(const mat4& M)
{
    if (!sel.empty()) doop(new wrl::modify_op(sel, M));
}

//-----------------------------------------------------------------------------

void wrl::world::init()
{
    sel = all;
    do_delete();
    edit_focus = 0;
}

void wrl::world::load(std::string name)
{
    wrl::atom *a;

    // Clear the selection in preparation for selecting all loaded entities.

    sel.clear();

    // Load the named file.

    try
    {
        app::file file(name);
        app::node root(file.get_root().find("world"));

        // Find all geom elements.

        for (app::node n = root.find("geom"); n; n = root.next(n, "geom"))
        {
            std::string type = n.get_s("type");

            // Create a new solid for each recognized geom type.

            if      (type == "sphere")    a = new wrl::sphere  (n);
            else if (type == "box")       a = new wrl::box     (n);
            else if (type == "plane")     a = new wrl::plane   (n);
            else if (type == "capsule")   a = new wrl::capsule (n);
            else if (type == "cylinder")  a = new wrl::cylinder(n);
            else if (type == "convex")    a = new wrl::convex  (n);
            else continue;

            // Select the new solid for addition to the world.

            sel.insert(a);
        }

        // Find all joint elements.

        for (app::node n = root.find("joint"); n; n = root.next(n, "joint"))
        {
            std::string type = n.get_s("type");

            // Create a new joint for each recognized joint type.

            if      (type == "ball")      a = new wrl::ball     (n);
            else if (type == "hinge")     a = new wrl::hinge    (n);
            else if (type == "hinge2")    a = new wrl::hinge2   (n);
            else if (type == "slider")    a = new wrl::slider   (n);
            else if (type == "amotor")    a = new wrl::amotor   (n);
            else if (type == "universal") a = new wrl::universal(n);
            else continue;

            // Select the new joint for addition to the world.

            sel.insert(a);
        }

        // Find all light elements.

        for (app::node n = root.find("light"); n; n = root.next(n, "light"))
        {
            std::string type = n.get_s("type");

            // Create a new light for each recognized light type.

            if      (type == "d-light") a = new wrl::d_light(n);
            else if (type == "s-light") a = new wrl::s_light(n);
            else continue;

            // Select the new light for addition to the world.

            sel.insert(a);
        }

        // Add the selected elements to the scene.

        do_create();

        // Ensure the body group serial number does not conflict.

        for (atom_set::iterator i = all.begin(); i != all.end(); ++i)
        {
            serial = std::max(serial, (*i)->body() + 1);
            serial = std::max(serial, (*i)->join() + 1);
        }
    }
    catch (std::exception& e)
    {
        etc::log(e.what());
    }
}

void wrl::world::save(std::string filename, bool save_all)
{
    // Construct a new XML DOM using from the current world.

    app::node head("?xml");
    app::node body("world");

    head.set_s("version", "1.0");
    head.set_s("?", "");

    body.insert(head);

    // Add some or all atoms to the DOM, as requested.

    if (save_all)
        for (atom_set::const_iterator i = all.begin(); i != all.end(); ++i)
            (*i)->save(body);
    else
        for (atom_set::const_iterator i = sel.begin(); i != sel.end(); ++i)
            (*i)->save(body);

    // Write the DOM to the named file.

    head.write(filename);
}

//-----------------------------------------------------------------------------

ogl::aabb wrl::world::prep_fill(int frusc, const app::frustum *const *frusv)
{
    // Set the highlight uniform.
#if 0
    GLfloat highlight = 0;

    if (edit_focus)
    {
        if (wrl::atom *a = (wrl::atom *) dGeomGetData(edit_focus))
        {
            if (ogl::unit *u = a->get_fill())
            {
                highlight = GLfloat(u->get_id());
            }
        }
    }
    uniform_highlight->set(highlight);
#endif
    // Prep the fill geometry pool.

    fill_pool->prep();

    // Cache the fill visibility and determine the visible bound.

    ogl::aabb bb;

    for (int frusi = 0; frusi < frusc; ++frusi)
        bb.merge(fill_pool->view(frusi, frusv[frusi]->get_world_planes(), 5));

    bb.inflate(1.01);
    return bb;
}

ogl::aabb wrl::world::prep_line(int frusc, const app::frustum *const *frusv)
{
    // Prep the line geometry pool.

    line_pool->prep();

    // Cache the line visibility and determine the visible bound.

    ogl::aabb bb;

    for (int frusi = 0; frusi < frusc; ++frusi)
        bb.merge(line_pool->view(frusi, frusv[frusi]->get_world_planes(), 5));

    bb.inflate(1.01);
    return bb;
}

//-----------------------------------------------------------------------------

// Set all light parameters and render the light source shadow map.

void wrl::world::set_light(int light, const vec4& p,
                           int frusi, app::frustum *frusp)
{
    // Bound the frustum to its visible volume.

    ogl::aabb bound = fill_pool->view(frusi, frusp->get_world_planes(), 5);

    frusp->set_bound(mat4(), bound);

    // Render the fill geometry to the shadow buffer.

    process_shadow[light]->bind_frame();
    {
        frusp->load_transform();

        glLoadIdentity();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        fill_pool->draw_init();
        {
            glCullFace(GL_FRONT);
            fill_pool->draw(frusi, false, false);
            fill_pool->draw(frusi, false, true);
            glCullFace(GL_BACK);
        }
        fill_pool->draw_fini();
    }
    process_shadow[light]->free_frame();

    // Set the position and transform uniforms.

    const mat4 P =  frusp->get_transform();
    const mat4 V = ::view->get_transform();
    const mat4 I = ::view->get_inverse();
    const mat4 S(0.5, 0.0, 0.0, 0.5,
                 0.0, 0.5, 0.0, 0.5,
                 0.0, 0.0, 0.5, 0.5,
                 0.0, 0.0, 0.0, 1.0);

    uniform_shadow[light]->set(S * P * I);
    uniform_light [light]->set(V * p);
}

// Add a spot light source.

int wrl::world::s_light(int light, const vec3& p, const vec3& v, double c,
                        int frusc, const app::frustum *const *frusv,
                                   const ogl::aabb& visible)
{
    if (light < 4)
    {
        app::perspective_frustum frust(p, -v, c, 1);
        set_light(light, vec4(p, 1), frusc + light, &frust);

        uniform_split[light]->set(vec2(0, 1));

        return 1;
    }
    return 0;
}

// Add a directional light source.

int wrl::world::d_light(int light, const vec3& p, const vec3& v, double c,
                        int frusc, const app::frustum *const *frusv,
                                   const ogl::aabb& visible)
{
    const int n = shadow_splits;

    for (int i = 0; i < n && light < 4; i++, light++)
    {
        // Compute the visible union of the bounds of this split.

        ogl::aabb bound;

        for (int frusi = 0; frusi < frusc; ++frusi)
            bound.merge(frusv[frusi]->get_split_bound(i, n));

        bound.intersect(visible);

        // Render a shadow map encompasing this bound.

        app::orthogonal_frustum frust(bound, v);
        set_light(light, vec4(v, 0), frusc + light, &frust);

        uniform_split[light]->set(vec2(double(i) / n, double(i + 1) / n));
    }
    return n;
}

void wrl::world::lite(int frusc, const app::frustum *const *frusv)
{
    // Determine the visible bounding volume. TODO: Remove this redundancy.

    ogl::aabb bound;

    for (int frusi = 0; frusi < frusc; ++frusi)
        bound.merge(fill_pool->view(frusi, frusv[frusi]->get_world_planes(), 5));

    // Enumerate the light sources.

    vec4 unit;
    vec4 spot;
    int l = 0;

    atom_set::iterator a;

    for (a = all.begin(); a != all.end() && (*a)->priority() < 0; ++a)
    {
        double c;
        vec2   b;

        // If this light is on...

        if ((c = (*a)->get_lighting(b)) > 0 && b[0] > 0)
        {
            if (ogl::unit *u = (*a)->get_fill())
            {
                // Generate light sources and render shadow maps.

                const ogl::binding *C = u->get_default_binding();
                const mat4          T = u->get_world_transform();

                const vec3 p = wvector(T);
                const vec3 v = yvector(T);

                int n = l;

                switch ((*a)->priority())
                {
                case -1: n += s_light(l, p, v, c, frusc, frusv, bound); break;
                case -2: n += d_light(l, p, v, c, frusc, frusv, bound); break;
                }

                // Set uniforms for the generated light sources.

                for (; l < n; l++)
                {
                    process_cookie[l]->draw(C);
                    uniform_bright[l]->set(b);

                    unit[l] = double(u->get_id());
                    spot[l] = c;
                }
            }
        }
    }

    uniform_spot->set(spot);
    uniform_unit->set(unit);

    // Zero the unused lights.

    for (; l < 4; l++)
        uniform_bright[l]->set(vec2(0, 0));
}

//-----------------------------------------------------------------------------

void wrl::world::draw_fill(int frusi, const app::frustum *frusp)
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fill_pool->draw_init();
    {
        glDisable(GL_BLEND);
        fill_pool->draw(frusi, true, false);
        glEnable(GL_BLEND);
        fill_pool->draw(frusi, true, true);
    }
    fill_pool->draw_fini();
}

void wrl::world::draw_line()
{
    // Render the line geometry.

    ogl::line_state_init();
    {
        line_pool->draw_init();
        {
            // Render the current selection.

            for (atom_set::iterator i = sel.begin(); i != sel.end(); ++i)
            {
                if ((*i)->body())
                    glColor3f(0.0f, 1.0f, 0.0f);
                else
                    glColor3f(1.0f, 0.0f, 0.0f);

                glLineWidth(4.0);
                (*i)->get_line()->draw_lines();
            }

            // Highlight the current focus.

            if (edit_focus)
            {
                if (wrl::atom *a = (wrl::atom *) dGeomGetData(edit_focus))
                {
                    glColor3f(1.0f, 1.0f, 0.0f);
                    glLineWidth(2.0);
                    a->get_line()->draw_lines();
                }
            }

            glColor3f(1.0f, 1.0f, 1.0f);
        }
        line_pool->draw_fini();
    }
    ogl::line_state_fini();
}

//-----------------------------------------------------------------------------
