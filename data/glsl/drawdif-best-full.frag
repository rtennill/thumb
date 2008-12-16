#extension GL_ARB_texture_rectangle : enable

//-----------------------------------------------------------------------------

uniform sampler2DRect cyl;
uniform vec2          cylk;
uniform vec2          cyld;

#include "glsl/mipmap-common.frag"
#include "glsl/mipmap0.frag"

//-----------------------------------------------------------------------------

void main()
{
    // Determine the coordinate of this pixel.

    vec2 c = texture2DRect(cyl, gl_FragCoord.xy).xy * cylk + cyld;

    gl_FragColor = cacheref0(c);
}

//-----------------------------------------------------------------------------