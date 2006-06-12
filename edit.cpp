#include <SDL.h>

#include "main.hpp"
#include "matrix.hpp"
#include "opengl.hpp"
#include "solid.hpp"
#include "joint.hpp"
#include "edit.hpp"

//-----------------------------------------------------------------------------

mode::edit::edit(ops::scene& s) : mode(s)
{
    // Initialize edit mode configuration.

    key_undo             = conf->get_i("key_undo");
    key_redo             = conf->get_i("key_redo");

    key_axis_X           = conf->get_i("key_axis_X");
    key_axis_Y           = conf->get_i("key_axis_Y");
    key_axis_Z           = conf->get_i("key_axis_Z");
    key_home             = conf->get_i("key_home");

    key_position_mode    = conf->get_i("key_position_mode");
    key_rotation_mode    = conf->get_i("key_rotation_mode");

    key_make_body        = conf->get_i("key_make_body");
    key_make_nonbody     = conf->get_i("key_make_nonbody");
    key_make_joint       = conf->get_i("key_make_joint");

    key_selection_delete = conf->get_i("key_selection_delete");
    key_selection_invert = conf->get_i("key_selection_invert");
    key_selection_extend = conf->get_i("key_selection_extend");
    key_selection_clear  = conf->get_i("key_selection_clear");
    key_selection_clone  = conf->get_i("key_selection_clone");

    grid_size            = conf->get_i("grid_size");

    // Initialize edit mode state.

    drag   = false;
    move   = false;

    point_p[0] =  0.0f;
    point_p[1] =  0.0f;
    point_p[2] =  0.0f;

    point_v[0] =  0.0f;
    point_v[1] =  0.0f;
    point_v[2] = -1.0f;
}

//-----------------------------------------------------------------------------

bool mode::edit::point(const float p[3], const float v[3], int, int)
{
    float M[16];

    point_p[0] = p[0];
    point_p[1] = p[1];
    point_p[2] = p[2];

    point_v[0] = v[0];
    point_v[1] = v[1];
    point_v[2] = v[2];

    if (drag)
    {
        // If the current drag has produced a previous transform, undo it.

        if (move)
        {
            scene.undo();
            move = false;
        }

        // Apply the transform of the current drag.

        if (transform.point(M, point_p, point_v))
        {
            scene.do_modify(M);
            move = true;
        }

        return true;
    }
    return false;
}

bool mode::edit::click(int b, bool d)
{
    if (b == 1)
    {
        ent::entity *focus = ent::entity::focused();
        float        M[16];

        drag = false;

        if (d)
        {
            // If a selected entity is clicked, a drag may be beginning.

/*          if (scene.selected(focus)) */
            {
                transform.click(point_p, point_v);
                drag = true;
            }
        }
        else
        {
            if (focus)
            {
                if (SDL_GetModState() & KMOD_SHIFT)
                {
                    // Shift-release resets the constraint transform.

                    if (SDL_GetModState() & KMOD_CTRL)
                        focus->get_local(M);
                    else
                        focus->get_world(M);

                    transform.set_transform(M);
                }
                else
                {
                    // A non-dragged release toggles selection.

                    if (move == false)
                        scene.click_selection(focus);
                }
            }
        }

        move = false;

        return true;
    }
    return false;
}

bool mode::edit::keybd(int k, bool d, int c)
{
    if (d)
    {
        // Handle basic editing operations.

        if      (k == key_selection_delete) scene.do_delete();
        else if (k == key_selection_invert) scene.invert_selection();
        else if (k == key_selection_extend) scene.extend_selection();
        else if (k == key_selection_clear)  scene.clear_selection();
        else if (k == key_selection_clone)  scene.clone_selection();

        else if (k == key_undo) scene.undo();
        else if (k == key_redo) scene.redo();

        // Handle body and joint creation and destruction.

        else if (k == key_make_body)    scene.do_embody();
        else if (k == key_make_nonbody) scene.do_debody();
        else if (k == key_make_joint)   scene.do_enjoin();

        // Handle constraint keys.

        else if ('0' <= k && k <= '9')   transform.set_grid(int(k - '0'));
        else if (k == key_position_mode) transform.set_mode(0);
        else if (k == key_rotation_mode) transform.set_mode(1);
        else if (k == key_axis_X)        transform.set_axis(0);
        else if (k == key_axis_Y)        transform.set_axis(1);
        else if (k == key_axis_Z)        transform.set_axis(2);

        else if (k == key_home)
        {
            float M[16];
            load_idt(M);
            transform.set_transform(M);
        }

        else return false;
    }
    else return false;

    return true;
}

//-----------------------------------------------------------------------------

void mode::edit::draw() const
{
    glPushAttrib(GL_ENABLE_BIT);
    {
        ent::set::iterator i;

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);

        // Draw the constraint.

        transform.draw(grid_size);

        // Draw the scene highlights.

        scene.draw_line();
        scene.draw_foci();
    }
    glPopAttrib();
}

//-----------------------------------------------------------------------------

