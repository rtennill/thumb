#ifndef OGL_FRAME_HPP
#define OGL_FRAME_HPP

#include <vector>

#include "ogl-opengl.hpp"

//-----------------------------------------------------------------------------

namespace ogl
{
    class frame
    {
        static std::vector<GLuint> stack;

        static void push(GLuint);
        static void pop();

        GLenum target;
        GLenum format;
        GLuint buffer;

        GLuint color;
        GLuint depth;

        bool has_depth;
        bool has_stencil;

        GLsizei w;
        GLsizei h;

    public:

        frame(GLsizei, GLsizei, GLenum=GL_TEXTURE_2D,
              GLenum=GL_RGBA8, bool=true, bool=false);

        virtual ~frame();

        void bind_color(GLenum=GL_TEXTURE0) const;
        void free_color(GLenum=GL_TEXTURE0) const;
        void bind_depth(GLenum=GL_TEXTURE0) const;
        void free_depth(GLenum=GL_TEXTURE0) const;

        virtual void bind(bool=false) const;
        virtual void free(bool=false) const;

        void draw()         const;
        void draw(int, int) const;
        void init();
        void fini();

        GLsizei get_w() const { return w; }
        GLsizei get_h() const { return h; }
    };
}

//-----------------------------------------------------------------------------

#endif
