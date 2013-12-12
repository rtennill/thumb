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

#include <cmath>
#include <cstring>

#include <SDL.h>
#include <SDL_keyboard.h>

#include <etc-vector.hpp>
#include <app-default.hpp>
#include <ogl-opengl.hpp>
#include <app-event.hpp>
#include <app-frustum.hpp>

//-----------------------------------------------------------------------------

static vec4 plane(const vec3& a, const vec3& b, const vec3& c)
{
    vec3 n = normal(cross(b - a, c - a));
    return vec4(n, -(n * a));
}

//-----------------------------------------------------------------------------

/// Constructor
///
/// This is the general-use frustum constructor. It receives an XML DOM node
/// for configuration serialization. The width and height parameters give the
/// resolution of a render target, if any.
///
/// By default, the frustum apex is placed at the origin (0, 0, 0), the view
/// transform is initialized to the identity, and the near and far distances
/// are set to 0.5 and 100.
///
/// \param node XML DOM node
/// \param w    Render target pixel width
/// \param h    Render target pixel height
///
app::frustum::frustum(app::node node, int w, int h) :
    node(node),
    pixel_w(w),
    pixel_h(h)
{
    // Load the configuration and perform a calibration.

    calc_calibrated();

    // Load sufficient defaults to allow immediate basic usage.

    set_viewpoint(user_pos);
    set_transform(mat4());
    set_distances(ogl::aabb(vec3(-10.0, -10.0, -10.0),
                            vec3(+10.0, +10.0, +10.0)));
}

/// Copy constuctor
///
/// This constructor copies the state of the given frustum object, including
/// any internally cached values, producing a functionally equivalent
/// frustum. However it does NOT copy the given frustum's XML DOM node
/// reference. This prevents a copy from overwriting the original's
/// serialization, but precludes all serialization by frustum copies.
///
/// \param that Frustum to be copied
///
app::frustum::frustum(const frustum& that) :
    node(0),
    pixel_w(that.pixel_w),
    pixel_h(that.pixel_h)
{
    user_pos   = that.user_pos;
    user_basis = that.user_basis;
    P          = that.P;

    memcpy(user_points, that.user_points, 4 * sizeof (vec3));
    memcpy(view_points, that.view_points, 8 * sizeof (vec3));
    memcpy(view_planes, that.view_planes, 6 * sizeof (vec4));
}

/// Lightsource projection constructor
///
/// \param v light source direction
/// \param b visible axis-aligned bounding volume
///
app::frustum::frustum(const vec3& v, const ogl::aabb& bound) :
    pixel_w(0),
    pixel_h(0)
{
    vec3 x(1, 0, 0);
    vec3 y(0, 1, 0);
    vec3 z = normal(v);

    // Compute a basis for the light orientation.

    if (fabs(z * y) < 1.0)
    {
        x = normal(cross(y, z));
        y = normal(cross(z, x));
    }
    else
    {
        y = normal(cross(z, x));
        x = normal(cross(y, z));
    }

    // These give the transform and inverse for the light's coordinate system.

    mat4 M = mat4(x[0], y[0], z[0], 0,
                  x[1], y[1], z[1], 0,
                  x[2], y[2], z[2], 0,
                  0,    0,    0,    1);
    mat4 I = mat4(x[0], x[1], x[2], 0,
                  y[0], y[1], y[2], 0,
                  z[0], z[1], z[2], 0,
                  0,    0,    0,    1);

    // Determine the light-space axis-aligned visible bounding volume

    ogl::aabb c(bound, I);

    vec3 p = c.get_min();
    vec3 q = c.get_max();

    // Compute the frustum corners.

    view_points[0] = M * vec3(p[0], p[1], q[2]);
    view_points[1] = M * vec3(q[0], p[1], q[2]);
    view_points[2] = M * vec3(p[0], q[1], q[2]);
    view_points[3] = M * vec3(q[0], q[1], q[2]);
    view_points[4] = M * vec3(p[0], p[1], p[2]);
    view_points[5] = M * vec3(q[0], p[1], p[2]);
    view_points[6] = M * vec3(p[0], q[1], p[2]);
    view_points[7] = M * vec3(q[0], q[1], p[2]);

    // Compute frustum planes for light view culling.

    view_planes[0] = plane(view_points[1], view_points[0], view_points[2]); // N
    view_planes[1] = plane(view_points[2], view_points[0], view_points[4]); // L
    view_planes[2] = plane(view_points[1], view_points[3], view_points[7]); // R
    view_planes[3] = plane(view_points[0], view_points[1], view_points[5]); // B
    view_planes[4] = plane(view_points[3], view_points[2], view_points[6]); // T

    // Compute the projection.

    P = orthogonal(p[0], q[0], p[1], q[1], -q[2], -p[2]) * I;
}

//-----------------------------------------------------------------------------

/// Set the projection matrix
///
/// Initialize the state of the frustum given a perspective projection matrix.
/// This is uncommon, but necessary for devices such as the Oculus Rift, which
/// report their configuration in this form.
///
/// \param M Perspective projection matrix

void app::frustum::set_projection(const mat4& M)
{
    const mat4 I = inverse(M);

    user_points[0] = I * vec4(-1, -1, -1,  1);
    user_points[1] = I * vec4( 1, -1, -1,  1);
    user_points[2] = I * vec4(-1,  1, -1,  1);
    user_points[3] = I * vec4( 1,  1, -1,  1);

    calc_basis();
}

/// Set the position of the apex of the view frustum.
///
/// The view frustum apex remains fixed for most applications, but can vary in
/// response to a user's movements within a user-tracked viewer-centric virtual
/// reality environment. Visibility testing must respond properly, and this
/// function makes that possible. The given position is usually the output of
/// the head sensor of a 3D tracking system and is given in the same user-space
/// coordinate system as the frustum corners.
///
/// \param p 3D position vector
///
void app::frustum::set_viewpoint(const vec3& p)
{
    user_pos = p;
}

/// Set the 3D view transformation.
///
/// The bounding volume of the frustum changes as the user moves about the
/// world. A great deal of visibility testing is performed using this volume,
/// so it pays off to cache its definition immediately.
///
/// \param M 4x4 transformation matrix.
///
void app::frustum::set_transform(const mat4& M)
{
    // Cache the world-space view position.

    view_pos = M * user_pos;

    // Cache the world-space display corners.

    view_points[0] = M * user_points[0];
    view_points[1] = M * user_points[1];
    view_points[2] = M * user_points[2];
    view_points[3] = M * user_points[3];

    // Cache the world-space view frustum bounding planes.

    view_planes[0] = plane(view_points[1], view_points[0], view_points[2]); // N
    view_planes[1] = plane(view_pos,       view_points[0], view_points[2]); // L
    view_planes[2] = plane(view_pos,       view_points[3], view_points[1]); // R
    view_planes[3] = plane(view_pos,       view_points[1], view_points[0]); // B
    view_planes[4] = plane(view_pos,       view_points[2], view_points[3]); // T

    // Force the near clipping plane to pass through the view point.

    view_planes[0][3] = -(view_pos * view_planes[0]);
}

/// Set the near and far clipping distances.
///
/// The five corners defining the off-axis pyramid of a view frustum define a
/// volume that is effectively infinite. However, perspective projection
/// requires a bounded volume, capped at the minimum and maximum visible
/// distance. This function sets these distances, and must be called prior to
/// the use of the frustum's perspective projection matrix.
///
/// \param b Axis-aligned bounding box
///
void app::frustum::set_distances(const ogl::aabb& bound)
{
    vec3 u[4];
    vec3 v[4];

    // Cache the near and far clipping plane distances.

    ogl::range nf = bound.get_range(view_planes[0]);

    double n = nf.get_n();
    double f = nf.get_f();

    n_dist = n;
    f_dist = f;

    // Compute the world-space frustum corner vectors.

    v[0] = normal(cross(view_planes[3], view_planes[1]));
    v[1] = normal(cross(view_planes[2], view_planes[3]));
    v[2] = normal(cross(view_planes[1], view_planes[4]));
    v[3] = normal(cross(view_planes[4], view_planes[2]));

    // For each view frustum corner...

    for (int i = 0; i < 4; ++i)
    {
        const double k = v[i] * view_planes[0];

        // Cache the world-space near and positions.

        view_points[i    ] = view_pos + v[i] * n / k;
        view_points[i + 4] = view_pos + v[i] * f / k;

        // Compute the user-space frustum corner vector.

        u[i] = normal(user_points[i] - user_pos);
    }

    // Generate the off-axis projection bounds.

    const double x0 = xvector(user_basis) * u[0];
    const double x1 = xvector(user_basis) * u[1];

    const double y0 = yvector(user_basis) * u[0];
    const double y2 = yvector(user_basis) * u[2];

    const double z0 = zvector(user_basis) * u[0];
    const double z1 = zvector(user_basis) * u[1];
    const double z2 = zvector(user_basis) * u[2];

    const double l = -n * x0 / z0;
    const double r = -n * x1 / z1;
    const double b = -n * y0 / z0;
    const double t = -n * y2 / z2;

    // The projection is the oriented perspective with apex at the origin.

    P = perspective(l, r, b, t, n, f)
      * mat4(transpose(user_basis))
      * translation(-user_pos);
}

//-----------------------------------------------------------------------------

/// Return the position of the apex of the view frustum in user coordinates.
///
const vec3 app::frustum::get_user_pos() const
{
    return user_pos;
}

/// Return the position of the apex of the view frustum in world coordinates.
///
const vec3 app::frustum::get_view_pos() const
{
    return view_pos;
}

/// Return the position of the apex of the view frustum in display coordinates.
///
const vec3 app::frustum::get_disp_pos() const
{
    return user_basis * (user_pos - (user_points[0] + user_points[3]) / 2.0);
}

/// Return the 4x4 perspective projection matrix in OpenGL form.
/// \warning If app::frustum::set_viewpoint has been called then
/// app::frustum::set_distances must also be called before the
/// perspective projection may be retrieved.
///
const mat4 app::frustum::get_perspective() const
{
    return P;
}

//-----------------------------------------------------------------------------

/// Return the user-space width of the base of the frustum in meters.
///
double app::frustum::get_w() const
{
    return length(user_points[1] - user_points[0]);
}

/// Return the user-space height of the base of the frustum in meters.
///
double app::frustum::get_h() const
{
    return length(user_points[2] - user_points[0]);
}

/// Return the width of the base of the frustum in pixels.
///
int app::frustum::get_pixel_w() const
{
    return pixel_w;
}

/// Return the height of the base of the frustum in pixels.
///
int app::frustum::get_pixel_h() const
{
    return pixel_h;
}

//-----------------------------------------------------------------------------

/// Compute and return the "practical" parallel-split shadow map coefficient,
/// the average of the theoretical solution and the worst possible solution.
/// \param i = split index
/// \param n = split count
///
double app::frustum::get_split_c(int i, int n) const
{
    return (n_dist * pow(f_dist / n_dist,   double(i) / double(n)) +
            n_dist +    (f_dist - n_dist) * double(i) / double(n)) / 2;
}

/// Compute and return the linear fraction of a parallel-split coefficient.
/// \param i = split index
/// \param n = split count
///
double app::frustum::get_split_k(int i, int n) const
{
    const double c = get_split_c(i, n);
    return (c - n_dist) / (f_dist - n_dist);
}

/// Compute and return the depth value of a parallel-split coefficient. This
/// is given in the same coordinate system as gl_FragCoord.z.
/// \param i = split index
/// \param n = split count
///
double app::frustum::get_split_z(int i, int n) const
{
    const double c = get_split_c(i, n);
    return (c - n_dist) / (f_dist - n_dist) * (f_dist / c);
}

/// Compute and return the axis-aligned bounding volume of a parallel-split
/// segment of the frustum.
/// \param i = split index
/// \param n = split count
///
ogl::aabb app::frustum::get_split_bound(int i, int n) const
{
    const double k0 = get_split_k(i,     n);
    const double k1 = get_split_k(i + 1, n);

    ogl::aabb b;

    b.merge(mix(view_points[0], view_points[4], k0));
    b.merge(mix(view_points[0], view_points[4], k1));
    b.merge(mix(view_points[1], view_points[5], k0));
    b.merge(mix(view_points[1], view_points[5], k1));
    b.merge(mix(view_points[2], view_points[6], k0));
    b.merge(mix(view_points[2], view_points[6], k1));
    b.merge(mix(view_points[3], view_points[7], k0));
    b.merge(mix(view_points[3], view_points[7], k1));

    return b;
}

//-----------------------------------------------------------------------------

bool app::frustum::pointer_to_2D(event *E, int& x, int& y) const
{
    vec3 p(E->data.point.p[0],
           E->data.point.p[1],
           E->data.point.p[2]);
    quat q(E->data.point.q[0],
           E->data.point.q[1],
           E->data.point.q[2],
           E->data.point.q[3]);

    // Determine the pointer vector from the quaternion.

    vec3 v = -zvector(mat3(q));

    // Determine where the pointer intersects with the image plane.

    vec4 n = plane(user_points[0], user_points[1], user_points[2]);

    double t = -((p * n) + n[3]) / (v * n);

    vec3 P =      p + v * t - user_points[0];
    vec3 X = user_points[1] - user_points[0];
    vec3 Y = user_points[2] - user_points[0];

    double xx = (P * X) / (X * X);
    double yy = (P * Y) / (Y * Y);

    // If the pointer falls within the frustum, return the nearest pixel.

    if (0 <= xx && xx <= 1 && 0 <= yy && yy <= 1)
    {
        x = toint(pixel_w * xx);
        y = toint(pixel_h * yy);
        return true;
    }
    return false;
}

bool app::frustum::pointer_to_3D(event *E, int x, int y) const
{
    double u = double(x) / double(pixel_w);
    double v = double(y) / double(pixel_h);
    double k = 1.0 - u - v;

    // Compute the Z axis of the pointer space.

    vec3 X = vec3(1, 0, 0);
    vec3 Y = vec3(0, 1, 0);
    vec3 Z = normal(user_pos - user_points[0] * k
                             - user_points[1] * u
                             - user_points[2] * v);

    // Complete an orthonormal basis of the pointer space.

    X = normal(cross(Y, Z));
    Y = normal(cross(Z, X));

    quat q(mat3(X, Y, Z));

    // Store the pointer origin and orientation in the event.

    E->mk_point(0, user_pos, q);
    return true;
}

bool app::frustum::process_event(app::event *E)
{
    if (E->get_type() == E_KEY && E->data.key.d)
    {
        const int k = E->data.key.k;
        const int m = E->data.key.m;

        if (m & KMOD_CTRL)
        {
            double d = ((m & KMOD_CAPS) || (m & KMOD_ALT)) ? 0.05 : 0.5;

            double dP = 0;
            double dT = 0;
            double dR = 0;
            double dp = 0;
            double dy = 0;
            double dr = 0;
            double dH = 0;
            double dV = 0;

            bool b = false;

            // Interpret the key event.

            if (m & KMOD_SHIFT)
            {
                if      (k == SDL_SCANCODE_LEFT)     { dT =  d; b = true; }
                else if (k == SDL_SCANCODE_RIGHT)    { dT = -d; b = true; }
                else if (k == SDL_SCANCODE_UP)       { dP =  d; b = true; }
                else if (k == SDL_SCANCODE_DOWN)     { dP = -d; b = true; }
                else if (k == SDL_SCANCODE_PAGEUP)   { dR =  d; b = true; }
                else if (k == SDL_SCANCODE_PAGEDOWN) { dR = -d; b = true; }
            }
            else
            {
                if      (k == SDL_SCANCODE_LEFT)     { dy = -d; b = true; }
                else if (k == SDL_SCANCODE_RIGHT)    { dy =  d; b = true; }
                else if (k == SDL_SCANCODE_UP)       { dp = -d; b = true; }
                else if (k == SDL_SCANCODE_DOWN)     { dp =  d; b = true; }
                else if (k == SDL_SCANCODE_PAGEUP)   { dr =  d; b = true; }
                else if (k == SDL_SCANCODE_PAGEDOWN) { dr = -d; b = true; }
                else if (k == SDL_SCANCODE_INSERT)   { dH =  d; b = true; }
                else if (k == SDL_SCANCODE_DELETE)   { dH = -d; b = true; }
                else if (k == SDL_SCANCODE_HOME)     { dV =  d; b = true; }
                else if (k == SDL_SCANCODE_END)      { dV = -d; b = true; }
            }

            // If changes occurred, apply them to the calibration.

            if (b)
            {
                double P, T, R, p, y, r, H, V;

                get_calibration(P, T, R, p, y, r, H, V);
                P += dP;
                T += dT;
                R += dR;
                p += dp;
                y += dy;
                r += dr;
                H += dH;
                V += dV;
                set_calibration(P, T, R, p, y, r, H, V);
                calc_calibrated();

                return true;
            }
        }
    }
    return false;
}

//-----------------------------------------------------------------------------

void app::frustum::load_transform() const
{
    // Load the projection to the OpenGL projection matrix.

    glMatrixMode(GL_PROJECTION);
    {
        glLoadMatrixd(transpose(P));
    }
    glMatrixMode(GL_MODELVIEW);

    // Clipping plane uniforms convey shadow map split depths.

    glPushMatrix();
    {
        glLoadIdentity();

        const int n = 3;
        for (int i = 0; i < n + 1; ++i)
            glClipPlane(GL_CLIP_PLANE0 + i, vec4(0, 0, 1, get_split_z(i, n)));
    }
    glPopMatrix();

    // (As built-in uniforms, clipping planes are visible to ALL programs.)
}

void app::frustum::apply_overlay() const
{
    // Produce a unit-to-pixel transformation for 2D overlay.

    glMatrixMode(GL_MODELVIEW);
    {
        glTranslated(user_points[0][0],
                     user_points[0][1],
                     user_points[0][2]);

        glMultMatrixd(transpose(mat4(user_basis)));

        glScaled(get_w() / pixel_w,
                 get_h() / pixel_h, 1.0);
    }
}

//-----------------------------------------------------------------------------

/// Extract the calibration from the serialization node.

void app::frustum::get_calibration(double& P, double& T, double& R,
                                   double& p, double& y, double& r,
                                   double& H, double& V)
{
    // Assign defaults for any undefined parameters.

    P = 0.0;               // Position phi
    T = 0.0;               // Position theta
    R = 0.0;               // Position rho
    p = 0.0;               // Rotation pitch
    y = 0.0;               // Rotation yaw
    r = 0.0;               // Rotation roll
    H = DEFAULT_HORZ_FOV;  // Horizontal field of view
    V = DEFAULT_VERT_FOV;  // Vertical   field of view

    if (node)
    {
        if (app::node n = node.find("position"))
        {
            P = n.get_f("p", P);
            T = n.get_f("t", T);
            R = n.get_f("r", R);
        }
        if (app::node n = node.find("rotation"))
        {
            p = n.get_f("p", p);
            y = n.get_f("y", y);
            r = n.get_f("r", r);
        }
        if (app::node n = node.find("perspective"))
        {
            H = n.get_f("hfov", H);
            V = n.get_f("vfov", V);
        }
    }
}

/// Update the calibration in the serialization node.

void app::frustum::set_calibration(double P, double T, double R,
                                   double p, double y, double r,
                                   double H, double V)
{
    if (node)
    {
        if (app::node n = node.find("position"))
        {
            n.set_f("p", P);
            n.set_f("t", T);
            n.set_f("r", R);
        }
        if (app::node n = node.find("rotation"))
        {
            n.set_f("p", p);
            n.set_f("y", y);
            n.set_f("r", r);
        }
        if (app::node n = node.find("perspective"))
        {
            n.set_f("hfov", H);
            n.set_f("vfov", V);
        }
    }
}

// Convert the current calibration to a transformation matrix.

mat4 app::frustum::mat_calibration()
{
    double P, T, R, p, y, r, H, V;

    get_calibration(P, T, R, p, y, r, H, V);

    return yrotation(to_radians(T))    // Position theta
         * xrotation(to_radians(P))    // Position phi
         * translation(vec3(0, 0, R))  // Position rho
         * yrotation(to_radians(y))    // Rotation yaw
         * xrotation(to_radians(p))    // Rotation pitch
         * zrotation(to_radians(r));   // Rotation roll
}

//-----------------------------------------------------------------------------

/// Compute screen corners given perspective fields-of-view.

void app::frustum::calc_corner_4(vec3& c0,
                                 vec3& c1,
                                 vec3& c2,
                                 vec3& c3, double H, double V)
{
    const double x = tan(to_radians(H * 0.5));
    const double y = tan(to_radians(V * 0.5));

    c0 = vec3(-x, -y, -1);
    c1 = vec3(+x, -y, -1);
    c2 = vec3(-x, +y, -1);
    c3 = vec3(+x, +y, -1);
}

/// Compute a fourth screen corner given three.

vec3 app::frustum::calc_corner_1(const vec3& a,
                                 const vec3& b,
                                 const vec3& c)
{
    return b + c - a;
}

/// Cache the display basis.

void app::frustum::calc_basis()
{
    vec3 x = normal(user_points[1] - user_points[0]);
    vec3 y = normal(user_points[2] - user_points[0]);
    vec3 z = normal(cross(x, y));

    user_basis = mat3(x[0], y[0], z[0],
                      x[1], y[1], z[1],
                      x[2], y[2], z[2]);
}

/// Extract the frustum definition from the serialization node.

void app::frustum::calc_calibrated()
{
    int  b = 0;
    vec3 c[4];
    mat4 T;

    double hfov = DEFAULT_HORZ_FOV;
    double vfov = DEFAULT_VERT_FOV;

    if (node)
    {
        // Extract the screen corners.

        for (app::node n = node.find("corner"); n; n = node.next(n, "corner"))
        {
            const std::string name = n.get_s("name");
            int i = 0;

            // Determine which corner is being specified.

            if (!name.empty())
            {
                if      (name == "BL") { i = 0; b |= 1; }
                else if (name == "BR") { i = 1; b |= 2; }
                else if (name == "TL") { i = 2; b |= 4; }
                else if (name == "TR") { i = 3; b |= 8; }
            }

            // Extract the position.

            const std::string unit = n.get_s("unit");

            double scale = scale_to_meters(unit.empty() ? "ft" : unit);

            c[i][0] = n.get_f("x") * scale;
            c[i][1] = n.get_f("y") * scale;
            c[i][2] = n.get_f("z") * scale;
        }

        // Extract fields-of-view.

        if (app::node n = node.find("perspective"))
        {
            hfov = n.get_f("hfov");
            vfov = n.get_f("vfov");
        }

        // Extract the calibration.

        T = mat_calibration();
    }

    // Compute any unspecified screen corner.

    if      (b == 14) c[0] = calc_corner_1(c[3], c[2], c[1]);
    else if (b == 13) c[1] = calc_corner_1(c[2], c[0], c[3]);
    else if (b == 11) c[2] = calc_corner_1(c[1], c[3], c[0]);
    else if (b ==  7) c[3] = calc_corner_1(c[0], c[1], c[2]);

    else calc_corner_4(c[0], c[1], c[2], c[3], hfov, vfov);

    // Apply the calibration transform to the configured frustum corners.

    user_points[0] = T * c[0];
    user_points[1] = T * c[1];
    user_points[2] = T * c[2];
    user_points[3] = T * c[3];

    calc_basis();
}

//-----------------------------------------------------------------------------
