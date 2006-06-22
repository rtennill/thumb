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

#include <map>
#include <cstdio>
#include <algorithm>

#include "opengl.hpp"
#include "matrix.hpp"
#include "scene.hpp"
#include "joint.hpp"
#include "solid.hpp"
#include "util.hpp"

//-----------------------------------------------------------------------------

// This entity set remains empty at all times. It is used as a return value
// when an undo or redo should cause the selection to be cleared.

namespace ops
{
    static ent::set deselection;
}

//-----------------------------------------------------------------------------

ops::create_op::create_op(ent::set& S) : operation(S)
{
}

ops::create_op::~create_op()
{
    // An undone create operation contains the only reference to the created
    // entities. If this operation is deleted before being redone then these
    // entities become abandoned and should be deleted.

    ent::set::iterator i;

    if (done == false)
        for (i = selection.begin(); i != selection.end(); ++i)
            delete (*i);
}

ent::set& ops::create_op::undo(scene *s)
{
    s->delete_set(selection);
    done = false;
    return deselection;
}

ent::set& ops::create_op::redo(scene *s)
{
    s->create_set(selection);
    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::delete_op::delete_op(ent::set& S) : operation(S)
{
}

ent::set& ops::delete_op::undo(scene *s)
{
    s->create_set(selection);
    done = false;
    return selection;
}

ent::set& ops::delete_op::redo(scene *s)
{
    s->delete_set(selection);
    done = true;
    return deselection;
}

//-----------------------------------------------------------------------------

ops::modify_op::modify_op(ent::set& S, const float M[16]) : operation(S)
{
    load_mat(T, M);
    load_inv(I, M);
}

ent::set& ops::modify_op::undo(scene *s)
{
    s->modify_set(selection, I);
    done = false;
    return selection;
}

ent::set& ops::modify_op::redo(scene *s)
{
    s->modify_set(selection, T);
    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::embody_op::embody_op(ent::set& S, int id) : operation(S), new_id(id)
{
    ent::set::iterator i;

    // Store the previous body IDs of all selected entities.

    for (i = selection.begin(); i != selection.end(); ++i)
        old_id[*i] = (*i)->body();
}

ent::set& ops::embody_op::undo(scene *s)
{
    ent::set::iterator i;

    // Reassign the previous body ID to each selected entitiy.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->body(old_id[*i]);

    done = false;
    return selection;
}

ent::set& ops::embody_op::redo(scene *s)
{
    ent::set::iterator i;

    // Assign the new body ID to each selected entity.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->body(new_id);

    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::enjoin_op::enjoin_op(ent::set& S) : operation(S), new_id(0)
{
    ent::set::iterator i;

    // Store the previous join IDs of all selected entities.

    for (i = selection.begin(); i != selection.end(); ++i)
        old_id[*i] = (*i)->join();

    // Scan the selection for potential joint targets.

    for (i = selection.begin(); i != selection.end(); ++i)
        new_id = MAX(new_id, (*i)->link());
}

ent::set& ops::enjoin_op::undo(scene *s)
{
    ent::set::iterator i;

    // Reassign the previous join ID to each selected entitiy.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->join(old_id[*i]);

    done = false;
    return selection;
}

ent::set& ops::enjoin_op::redo(scene *s)
{
    ent::set::iterator i;

    // Assign the new join ID to each selected entity.

    for (i = selection.begin(); i != selection.end(); ++i)
        (*i)->join(new_id);

    done = true;
    return selection;
}

//-----------------------------------------------------------------------------

ops::scene::scene(ent::entity& camera) : camera(camera), serial(1)
{
}

ops::scene::~scene()
{
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
}

//-----------------------------------------------------------------------------

bool ops::scene::selected(ent::entity *e)
{
    return (sel.find(e) != sel.end());
}

bool ops::scene::selected()
{
    return (!sel.empty());
}

void ops::scene::click_selection(ent::entity *e)
{
    if (selected(e))
        sel.erase(e);
    else
        sel.insert(e);
}

void ops::scene::clone_selection()
{
    ent::set::iterator i;
    ent::set clones;

    // Remove all selected entities.

    for (i = sel.begin(); i != sel.end(); ++i)
        clones.insert((*i)->clone());

    // Select the clones and push an undo-able create operation for them.

    sel = clones;
    do_create();
}

void ops::scene::clear_selection()
{
    sel.clear();
}

//-----------------------------------------------------------------------------

void ops::scene::invert_selection()
{
    // Begin with the set of all entities.

    ent::set unselected;
    ent::set::iterator i;

    unselected = all;

    // Remove all selected entities.

    for (i = sel.begin(); i != sel.end(); ++i)
        unselected.erase(unselected.find(*i));

    // Giving the set of all unselected entities.

    sel = unselected;
}

void ops::scene::extend_selection()
{
    std::set<int> bodies;

    // Define a set of the body IDs of all selected entities.

    for (ent::set::iterator i = sel.begin(); i != sel.end(); ++i)
        bodies.insert((*i)->body());

    // Add all entities with an included body ID to the selection.

    for (ent::set::iterator j = all.begin(); j != all.end(); ++j)
    {
        if (bodies.find((*j)->body()) != bodies.end())
            sel.insert(*j);
        if (bodies.find((*j)->join()) != bodies.end())
            sel.insert(*j);
    }
}

void ops::scene::create_set(ent::set& set)
{
    ent::set::iterator i;

    // Add all given entities to the entity set.

    for (i = set.begin(); i != set.end(); ++i)
    {
        all.insert(*i);
        (*i)->edit_init();
    }
}

void ops::scene::delete_set(ent::set& set)
{
    ent::set::iterator i;

    // Remove all given entities from the entity set.

    for (i = set.begin(); i != set.end(); ++i)
    {
        all.erase(all.find(*i));
        (*i)->edit_fini();
    }
}

void ops::scene::modify_set(ent::set& set, const float T[16])
{
    ent::set::iterator i;

    // Apply the given transform to all given entities.

    for (i = set.begin(); i != set.end(); ++i)
    {
        (*i)->mult_world(T);
        (*i)->set_default();
    }
}

//-----------------------------------------------------------------------------

void ops::scene::embody_all()
{
    ent::set::iterator i;
    body_map::iterator j;

    int id;

    // Assign a body to each active entity group.

    for (i = all.begin(); i != all.end(); ++i)
        if ((id = (*i)->body()))
        {
            if (bodies[id] == 0)
                bodies[id] = ent::entity::phys_body();
        }

    // Add each active entity to its body.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->play_init(bodies[(*i)->body()]);

    // Center each body on its center of mass.

    for (j = bodies.begin(); j != bodies.end(); ++j)
        if (j->second)
        {
            dMass mass;

            dBodyGetMass(j->second, &mass);

            dBodySetPosition(j->second, +mass.c[0], +mass.c[1], +mass.c[2]);
            dMassTranslate      (&mass, -mass.c[0], -mass.c[1], -mass.c[2]);

            dBodySetMass(j->second, &mass);
        }

    // Set the transform geom of each solid entity.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->play_tran(bodies[(*i)->body()]);

    // Set the joint target of each joint entity.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->play_join(bodies[(*i)->join()]);
}

void ops::scene::debody_all()
{
    ent::set::iterator i;
    body_map::iterator j;

    // Revert the state of all active entities.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->play_fini();

    // Destroy all bodies.

    for (j = bodies.begin(); j != bodies.end(); ++j)
        if (j->second)
            dBodyDestroy(j->second);

    bodies.clear();
}

void ops::scene::set_param(int k, std::string& expr)
{
    ent::set::iterator i;

    for (i = sel.begin(); i != sel.end(); ++i)
        (*i)->set_param(k, expr);
}

int ops::scene::get_param(int k, std::string& expr)
{
    ent::set::iterator i;
    std::set<std::string> values;

    // Determine the number of distinct values among the selection's params.

    for (i = sel.begin(); i != sel.end(); ++i)
        if ((*i)->get_param(k, expr))
            values.insert(expr);

    return int(values.size());
}

//-----------------------------------------------------------------------------

void ops::scene::doop(operation_p op)
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

    op->redo(this);
}

void ops::scene::undo()
{
    // Undo an operation and move it to the redo-able list.

    if (!undo_list.empty())
    {
        sel = undo_list.front()->undo(this);

        redo_list.push_front(undo_list.front());
        undo_list.pop_front();
    }
}

void ops::scene::redo()
{
    // Redo an operation and move it to the undo-able list.

    if (!redo_list.empty())
    {
        sel = redo_list.front()->redo(this);

        undo_list.push_front(redo_list.front());
        redo_list.pop_front();
    }
}

//-----------------------------------------------------------------------------

void ops::scene::do_create()
{
    // Ensure that the new entity body IDs do not conflict with existing ones.

    std::set<int> A;
    std::set<int> B;
    std::set<int> C;

    ent::set::iterator i;
    ent::set::iterator j;

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

    if (!sel.empty()) doop(new create_op(sel));
}

void ops::scene::do_delete()
{
    if (!sel.empty())
    {
        doop(new delete_op(sel));
        sel.clear();
    }
}

void ops::scene::do_enjoin()
{
    if (!sel.empty()) doop(new enjoin_op(sel));
}

void ops::scene::do_embody()
{
    if (!sel.empty()) doop(new embody_op(sel, serial++));
}

void ops::scene::do_debody()
{
    if (!sel.empty()) doop(new embody_op(sel, 0));
}

void ops::scene::do_modify(const float M[16])
{
    if (!sel.empty()) doop(new modify_op(sel, M));
}

//-----------------------------------------------------------------------------

static mxml_type_t load_cb(mxml_node_t *node)
{
    std::string name(node->value.element.name);

    if (name == "param") return MXML_OPAQUE;

    if (name == "world") return MXML_ELEMENT;
    if (name == "joint") return MXML_ELEMENT;
    if (name == "geom")  return MXML_ELEMENT;

    if (name == "body1") return MXML_INTEGER;
    if (name == "body2") return MXML_INTEGER;

    if (name == "rot_x") return MXML_REAL;
    if (name == "rot_y") return MXML_REAL;
    if (name == "rot_z") return MXML_REAL;
    if (name == "rot_w") return MXML_REAL;

    if (name == "pos_x") return MXML_REAL;
    if (name == "pos_y") return MXML_REAL;
    if (name == "pos_z") return MXML_REAL;

    return MXML_TEXT;
}

static const char *save_cb(mxml_node_t *node, int where)
{
    std::string name(node->value.element.name);

    switch (where)
    {
    case MXML_WS_AFTER_OPEN:
        if (name == "?xml")  return "\n";
        if (name == "geom")  return "\n";
        if (name == "joint") return "\n";
        if (name == "world") return "\n";
        break;

    case MXML_WS_BEFORE_OPEN:
        if (name == "geom")  return "  ";
        if (name == "joint") return "  ";

        if (name == "file")  return "    ";
        if (name == "param") return "    ";
        if (name == "body1") return "    ";
        if (name == "body2") return "    ";

        if (name == "rot_x") return "    ";
        if (name == "rot_y") return "    ";
        if (name == "rot_z") return "    ";
        if (name == "rot_w") return "    ";

        if (name == "pos_x") return "    ";
        if (name == "pos_y") return "    ";
        if (name == "pos_z") return "    ";
        break;

    case MXML_WS_BEFORE_CLOSE:
        if (name == "geom")  return "  ";
        if (name == "joint") return "  ";
        break;

    case MXML_WS_AFTER_CLOSE:
        return "\n";
    }

    return NULL;
}

//-----------------------------------------------------------------------------

void ops::scene::init()
{
    sel = all;
    do_delete();
}

void ops::scene::load(std::string filename)
{
    FILE *fp;

    // Clear the selection in preparation for selecting all loaded entities.

    sel.clear();

    // Load the named file.

    if ((fp = fopen(filename.c_str(), "r")))
    {
        mxml_node_t *n;
        mxml_node_t *H = mxmlLoadFile(0, fp, load_cb);
        mxml_node_t *T = mxmlFindElement(H, H, "world", 0, 0,
                                         MXML_DESCEND_FIRST);
        ent::joint *j = 0;
        ent::solid *s = 0;

        // Find all geom elements.

        for (n = mxmlFindElement(T, T, "geom", 0, 0, MXML_DESCEND_FIRST); n;
             n = mxmlFindElement(n, T, "geom", 0, 0, MXML_NO_DESCEND))

            if (mxmlElementGetAttr(n, "class"))
            {
                std::string type(mxmlElementGetAttr(n, "class"));

                // Create a new solid for each recognized geom class.

                if      (type == "box")     s = new ent::box;
                else if (type == "sphere")  s = new ent::sphere;
                else if (type == "capsule") s = new ent::capsule;
                else                        continue;

                // Allow the new solid to parse its own attributes.

                s->load(n);

                // Select the new solid for addition to the scene.

                sel.insert(s);
            }

        // Find all joint elements.

        for (n = mxmlFindElement(T, T, "joint", 0, 0, MXML_DESCEND_FIRST); n;
             n = mxmlFindElement(n, T, "joint", 0, 0, MXML_NO_DESCEND))

            if (mxmlElementGetAttr(n, "type"))
            {
                std::string type(mxmlElementGetAttr(n, "type"));

                // Create a new joint for each recognized joint type.

                if      (type == "ball")      j = new ent::ball;
                else if (type == "hinge")     j = new ent::hinge;
                else if (type == "hinge2")    j = new ent::hinge2;
                else if (type == "slider")    j = new ent::slider;
                else if (type == "amotor")    j = new ent::amotor;
                else if (type == "universal") j = new ent::universal;
                else                          continue;

                // Allow the new joint to parse its own attributes.

                j->load(n);

                // Select the new joint for addition to the scene.

                sel.insert(j);
            }

        // Add the selected elements to the scene.

        do_create();

        // Ensure the body group serial number does not conflict.

        for (ent::set::iterator i = all.begin(); i != all.end(); ++i)
        {
            serial = MAX(serial, (*i)->body() + 1);
            serial = MAX(serial, (*i)->join() + 1);
        }

        mxmlDelete(H);
        fclose(fp);
    }
}

//-----------------------------------------------------------------------------

void ops::scene::save(std::string filename, bool save_all)
{
    ent::set::const_iterator b = sel.begin();
    ent::set::const_iterator e = sel.end();

    ent::set save;

    FILE *fp;

    if (save_all)
    {
        save = all;

        b = save.begin();
        e = save.end();
    }

    if ((fp = fopen(filename.c_str(), "w")))
    {
        mxml_node_t *head = mxmlNewElement(0, "?xml");
        mxml_node_t *root = mxmlNewElement(head, "world");

        mxmlElementSetAttr(head, "version", "1.0");
        mxmlElementSetAttr(head, "?", 0);

        for (ent::set::const_iterator i = b; i != e; ++i)
            (*i)->save(root);

        mxmlSaveFile(head, fp, save_cb);
        mxmlDelete(head);

        fclose(fp);
    }
}

//-----------------------------------------------------------------------------

void ops::scene::step(float dt)
{
    ent::set::iterator i;

    // Preprocess each active entity.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->step_prep();

    // Step.

    ent::entity::phys_step(dt);

    // Postprocess each active entity.

    for (i = all.begin(); i != all.end(); ++i)
        (*i)->step_post();
}

void ops::scene::draw(int flags) const
{
    typedef std::pair    <int, const ent::entity *> pair;
    typedef std::multimap<int, const ent::entity *> mmap;

    mmap V;
    mmap D;

    mmap::const_iterator i;
    mmap::const_iterator j;

    // Construct prioritized lists of viewers and drawers.

    ent::set::iterator e;
    int p;

    for (e = all.begin(); e != all.end(); ++e)
    {
        if ((p = (*e)->view_prio(flags)) > 0) V.insert(pair(p, *e));
        if ((p = (*e)->draw_prio(flags)) > 0) D.insert(pair(p, *e));
    }

    // If the scene includes no viewers, include the default camera.

    if (V.empty())
        V.insert(pair(1, &camera));

    // Preprocess each drawer.

    for (j = D.begin(); j != D.end(); ++j)
        j->second->draw_prep(flags);

    // Draw all drawers to all viewers.

    for (i = V.begin(); i != V.end(); ++i)
    {
        i->second->view_prep(flags);

        for (j = D.begin(); j != D.end(); ++j)
            j->second->draw(flags);

        i->second->view_post(flags);
    }
            
    // Postprocess each drawer.

    for (j = D.begin(); j != D.end(); ++j)
        j->second->draw_post(flags);
}

//-----------------------------------------------------------------------------
