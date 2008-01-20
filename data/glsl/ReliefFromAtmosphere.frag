#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect dif;
uniform sampler2DRect nrm;

uniform vec2 frag_k;
uniform vec2 frag_d;
varying float day;

void main (void)
{
    vec2 p = (gl_FragCoord.xy + frag_d) * frag_k;

    vec3 L = gl_LightSource[0].position.xyz;
    vec3 D = texture2DRect(dif, p).rgb;
    vec3 N = texture2DRect(nrm, p).rgb;

    N = normalize((N * 2.0) - 1.0);

    vec3 R = vec3(max(dot(N, L), 0.0));

    vec3 d = max(dot(N, L), 0.0) * gl_LightSource[0].diffuse.rgb;
//  vec3 a =                       gl_LightModel.ambient.rgb;
    vec3 a = vec3(day) *           gl_LightModel.ambient.rgb;

    gl_FragColor = vec4(R, 1.0);
}