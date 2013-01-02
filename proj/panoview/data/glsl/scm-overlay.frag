#version 120

struct scm
{
    sampler2D S;
    vec2      r;
    vec2      b[16];
    float     a[16];
    float     k0;
    float     k1;
};

uniform scm lower;
uniform scm upper;

uniform vec2 A[16];
uniform vec2 B[16];

//------------------------------------------------------------------------------

vec4 mux(vec4 a, vec4 b)
{
    return mix(a, b, floor(b.a));
}

vec4 sample_lower(vec2 t)
{
    vec4   c = texture2D(lower.S, (t * A[ 0] + B[ 0]) * lower.r + lower.b[ 0]);
    c = mix(c, texture2D(lower.S, (t * A[ 1] + B[ 1]) * lower.r + lower.b[ 1]), lower.a[ 1]);
    c = mix(c, texture2D(lower.S, (t * A[ 2] + B[ 2]) * lower.r + lower.b[ 2]), lower.a[ 2]);
    c = mix(c, texture2D(lower.S, (t * A[ 3] + B[ 3]) * lower.r + lower.b[ 3]), lower.a[ 3]);
    c = mix(c, texture2D(lower.S, (t * A[ 4] + B[ 4]) * lower.r + lower.b[ 4]), lower.a[ 4]);
    c = mix(c, texture2D(lower.S, (t * A[ 5] + B[ 5]) * lower.r + lower.b[ 5]), lower.a[ 5]);
    c = mix(c, texture2D(lower.S, (t * A[ 6] + B[ 6]) * lower.r + lower.b[ 6]), lower.a[ 6]);
    c = mix(c, texture2D(lower.S, (t * A[ 7] + B[ 7]) * lower.r + lower.b[ 7]), lower.a[ 7]);
    c = mix(c, texture2D(lower.S, (t * A[ 8] + B[ 8]) * lower.r + lower.b[ 8]), lower.a[ 8]);
    c = mix(c, texture2D(lower.S, (t * A[ 9] + B[ 9]) * lower.r + lower.b[ 9]), lower.a[ 9]);
    c = mix(c, texture2D(lower.S, (t * A[10] + B[10]) * lower.r + lower.b[10]), lower.a[10]);
    c = mix(c, texture2D(lower.S, (t * A[11] + B[11]) * lower.r + lower.b[11]), lower.a[11]);
    c = mix(c, texture2D(lower.S, (t * A[12] + B[12]) * lower.r + lower.b[12]), lower.a[12]);
    c = mix(c, texture2D(lower.S, (t * A[13] + B[13]) * lower.r + lower.b[13]), lower.a[13]);
    c = mix(c, texture2D(lower.S, (t * A[14] + B[14]) * lower.r + lower.b[14]), lower.a[14]);
    c = mix(c, texture2D(lower.S, (t * A[15] + B[15]) * lower.r + lower.b[15]), lower.a[15]);
    return c;
}

vec4 sample_upper(vec2 t)
{
    vec4   c = texture2D(upper.S, (t * A[ 0] + B[ 0]) * upper.r + upper.b[ 0]);
    c = mux(c, texture2D(upper.S, (t * A[ 1] + B[ 1]) * upper.r + upper.b[ 1]));
    c = mux(c, texture2D(upper.S, (t * A[ 2] + B[ 2]) * upper.r + upper.b[ 2]));
    c = mux(c, texture2D(upper.S, (t * A[ 3] + B[ 3]) * upper.r + upper.b[ 3]));
    c = mux(c, texture2D(upper.S, (t * A[ 4] + B[ 4]) * upper.r + upper.b[ 4]));
    c = mux(c, texture2D(upper.S, (t * A[ 5] + B[ 5]) * upper.r + upper.b[ 5]));
    c = mux(c, texture2D(upper.S, (t * A[ 6] + B[ 6]) * upper.r + upper.b[ 6]));
    c = mux(c, texture2D(upper.S, (t * A[ 7] + B[ 7]) * upper.r + upper.b[ 7]));
    c = mux(c, texture2D(upper.S, (t * A[ 8] + B[ 8]) * upper.r + upper.b[ 8]));
    c = mux(c, texture2D(upper.S, (t * A[ 9] + B[ 9]) * upper.r + upper.b[ 9]));
    c = mux(c, texture2D(upper.S, (t * A[10] + B[10]) * upper.r + upper.b[10]));
    c = mux(c, texture2D(upper.S, (t * A[11] + B[11]) * upper.r + upper.b[11]));
    c = mux(c, texture2D(upper.S, (t * A[12] + B[12]) * upper.r + upper.b[12]));
    c = mux(c, texture2D(upper.S, (t * A[13] + B[13]) * upper.r + upper.b[13]));
    c = mux(c, texture2D(upper.S, (t * A[14] + B[14]) * upper.r + upper.b[14]));
    c = mux(c, texture2D(upper.S, (t * A[15] + B[15]) * upper.r + upper.b[15]));
    return c;
}

//------------------------------------------------------------------------------

void main()
{
    vec4 a = sample_lower(gl_TexCoord[0].xy);
    vec4 b = sample_upper(gl_TexCoord[0].xy);

    gl_FragColor = mux(a, b);
}
