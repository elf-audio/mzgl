#pragma once
/*
    #version:1# (machine generated, don't edit!)

    Generated by sokol-shdc (https://github.com/floooh/sokol-tools)

    Cmdline:
        sokol-shdc --reflection --input /Users/marek/koala/src/shaders/slider.glsl --output /Users/marek/koala/cmake-build-debug/slider.glsl.h --slang glsl300es:hlsl5:metal_macos

    Overview:
    =========
    Shader program: 'slider':
        Get shader desc: slider_shader_desc(sg_query_backend());
        Vertex shader: vs
            Attributes:
                ATTR_vs_Position => 0
                ATTR_vs_TexCoord => 1
            Uniform block 'vertParams':
                C struct: vertParams_t
                Bind slot: SLOT_vertParams => 0
        Fragment shader: fs
            Uniform block 'fragParams':
                C struct: fragParams_t
                Bind slot: SLOT_fragParams => 0
*/
#if !defined(SOKOL_GFX_INCLUDED)
#error "Please include sokol_gfx.h before slider.glsl.h"
#endif
#if !defined(SOKOL_SHDC_ALIGN)
#if defined(_MSC_VER)
#define SOKOL_SHDC_ALIGN(a) __declspec(align(a))
#else
#define SOKOL_SHDC_ALIGN(a) __attribute__((aligned(a)))
#endif
#endif
#define ATTR_vs_Position (0)
#define ATTR_vs_TexCoord (1)
#define SLOT_vertParams (0)
#define SLOT_fragParams (0)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct vertParams_t {
    float mvp[16];
    float color[4];
    float val;
    float centre;
    float vertical;
    uint8_t _pad_92[4];
} vertParams_t;
#pragma pack(pop)
#pragma pack(push,1)
SOKOL_SHDC_ALIGN(16) typedef struct fragParams_t {
    float bgColor[4];
} fragParams_t;
#pragma pack(pop)
/*
    #version 300 es

    uniform vec4 vertParams[6];
    out float tcx;
    layout(location = 1) in vec2 TexCoord;
    out vec4 colorV;
    out float minAmt;
    out float maxAmt;
    layout(location = 0) in vec4 Position;

    void main()
    {
        tcx = TexCoord.x * (1.0 - vertParams[5].z) + (TexCoord.y * vertParams[5].z);
        colorV = vertParams[4];
        minAmt = min(vertParams[5].x, vertParams[5].y);
        maxAmt = max(vertParams[5].x, vertParams[5].y);
        gl_Position = mat4(vertParams[0], vertParams[1], vertParams[2], vertParams[3]) * Position;
    }

*/
static const uint8_t vs_source_glsl300es[518] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x0a,0x75,0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x76,0x65,0x63,0x34,0x20,0x76,0x65,
    0x72,0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x36,0x5d,0x3b,0x0a,0x6f,0x75,0x74,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x74,0x63,0x78,0x3b,0x0a,0x6c,0x61,0x79,0x6f,
    0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x31,0x29,
    0x20,0x69,0x6e,0x20,0x76,0x65,0x63,0x32,0x20,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,
    0x64,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x76,0x65,0x63,0x34,0x20,0x63,0x6f,0x6c,0x6f,
    0x72,0x56,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x69,
    0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x6f,0x75,0x74,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,
    0x6d,0x61,0x78,0x41,0x6d,0x74,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,
    0x6f,0x63,0x61,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x69,0x6e,0x20,
    0x76,0x65,0x63,0x34,0x20,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x0a,
    0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,
    0x20,0x20,0x74,0x63,0x78,0x20,0x3d,0x20,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,
    0x2e,0x78,0x20,0x2a,0x20,0x28,0x31,0x2e,0x30,0x20,0x2d,0x20,0x76,0x65,0x72,0x74,
    0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x7a,0x29,0x20,0x2b,0x20,0x28,
    0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x2e,0x79,0x20,0x2a,0x20,0x76,0x65,0x72,
    0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x7a,0x29,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x3d,0x20,0x76,0x65,0x72,0x74,
    0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x34,0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x6d,
    0x69,0x6e,0x41,0x6d,0x74,0x20,0x3d,0x20,0x6d,0x69,0x6e,0x28,0x76,0x65,0x72,0x74,
    0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x78,0x2c,0x20,0x76,0x65,0x72,
    0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x79,0x29,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x20,0x3d,0x20,0x6d,0x61,0x78,0x28,
    0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x78,0x2c,
    0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x35,0x5d,0x2e,0x79,
    0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x20,0x3d,0x20,0x6d,0x61,0x74,0x34,0x28,0x76,0x65,0x72,0x74,0x50,0x61,
    0x72,0x61,0x6d,0x73,0x5b,0x30,0x5d,0x2c,0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,
    0x61,0x6d,0x73,0x5b,0x31,0x5d,0x2c,0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,
    0x6d,0x73,0x5b,0x32,0x5d,0x2c,0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,0x6d,
    0x73,0x5b,0x33,0x5d,0x29,0x20,0x2a,0x20,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    #version 300 es
    precision mediump float;
    precision highp int;

    uniform highp vec4 fragParams[1];
    in highp float minAmt;
    in highp float tcx;
    in highp float maxAmt;
    layout(location = 0) out highp vec4 fragColor;
    in highp vec4 colorV;

    void main()
    {
        highp float _19 = step(minAmt, tcx) - step(maxAmt, tcx);
        fragColor = (colorV * _19) + (fragParams[0] * (1.0 - _19));
    }

*/
static const uint8_t fs_source_glsl300es[376] = {
    0x23,0x76,0x65,0x72,0x73,0x69,0x6f,0x6e,0x20,0x33,0x30,0x30,0x20,0x65,0x73,0x0a,
    0x70,0x72,0x65,0x63,0x69,0x73,0x69,0x6f,0x6e,0x20,0x6d,0x65,0x64,0x69,0x75,0x6d,
    0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x3b,0x0a,0x70,0x72,0x65,0x63,0x69,0x73,0x69,
    0x6f,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x69,0x6e,0x74,0x3b,0x0a,0x0a,0x75,
    0x6e,0x69,0x66,0x6f,0x72,0x6d,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,
    0x34,0x20,0x66,0x72,0x61,0x67,0x50,0x61,0x72,0x61,0x6d,0x73,0x5b,0x31,0x5d,0x3b,
    0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,
    0x6d,0x69,0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x74,0x63,0x78,0x3b,0x0a,0x69,0x6e,0x20,0x68,
    0x69,0x67,0x68,0x70,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,
    0x74,0x3b,0x0a,0x6c,0x61,0x79,0x6f,0x75,0x74,0x28,0x6c,0x6f,0x63,0x61,0x74,0x69,
    0x6f,0x6e,0x20,0x3d,0x20,0x30,0x29,0x20,0x6f,0x75,0x74,0x20,0x68,0x69,0x67,0x68,
    0x70,0x20,0x76,0x65,0x63,0x34,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,0x72,
    0x3b,0x0a,0x69,0x6e,0x20,0x68,0x69,0x67,0x68,0x70,0x20,0x76,0x65,0x63,0x34,0x20,
    0x63,0x6f,0x6c,0x6f,0x72,0x56,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x6d,0x61,
    0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x68,0x69,0x67,0x68,0x70,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x31,0x39,0x20,0x3d,0x20,0x73,0x74,0x65,
    0x70,0x28,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x2c,0x20,0x74,0x63,0x78,0x29,0x20,0x2d,
    0x20,0x73,0x74,0x65,0x70,0x28,0x6d,0x61,0x78,0x41,0x6d,0x74,0x2c,0x20,0x74,0x63,
    0x78,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,
    0x72,0x20,0x3d,0x20,0x28,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x2a,0x20,0x5f,0x31,
    0x39,0x29,0x20,0x2b,0x20,0x28,0x66,0x72,0x61,0x67,0x50,0x61,0x72,0x61,0x6d,0x73,
    0x5b,0x30,0x5d,0x20,0x2a,0x20,0x28,0x31,0x2e,0x30,0x20,0x2d,0x20,0x5f,0x31,0x39,
    0x29,0x29,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
/*
    cbuffer vertParams : register(b0)
    {
        row_major float4x4 _22_mvp : packoffset(c0);
        float4 _22_color : packoffset(c4);
        float _22_val : packoffset(c5);
        float _22_centre : packoffset(c5.y);
        float _22_vertical : packoffset(c5.z);
    };


    static float4 gl_Position;
    static float tcx;
    static float2 TexCoord;
    static float4 colorV;
    static float minAmt;
    static float maxAmt;
    static float4 Position;

    struct SPIRV_Cross_Input
    {
        float4 Position : TEXCOORD0;
        float2 TexCoord : TEXCOORD1;
    };

    struct SPIRV_Cross_Output
    {
        float4 colorV : TEXCOORD0;
        float tcx : TEXCOORD1;
        float minAmt : TEXCOORD2;
        float maxAmt : TEXCOORD3;
        float4 gl_Position : SV_Position;
    };

    void vert_main()
    {
        tcx = mad(TexCoord.x, 1.0f - _22_vertical, TexCoord.y * _22_vertical);
        colorV = _22_color;
        minAmt = min(_22_val, _22_centre);
        maxAmt = max(_22_val, _22_centre);
        gl_Position = mul(Position, _22_mvp);
    }

    SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
    {
        TexCoord = stage_input.TexCoord;
        Position = stage_input.Position;
        vert_main();
        SPIRV_Cross_Output stage_output;
        stage_output.gl_Position = gl_Position;
        stage_output.tcx = tcx;
        stage_output.colorV = colorV;
        stage_output.minAmt = minAmt;
        stage_output.maxAmt = maxAmt;
        return stage_output;
    }
*/
static const uint8_t vs_source_hlsl5[1320] = {
    0x63,0x62,0x75,0x66,0x66,0x65,0x72,0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,
    0x6d,0x73,0x20,0x3a,0x20,0x72,0x65,0x67,0x69,0x73,0x74,0x65,0x72,0x28,0x62,0x30,
    0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x72,0x6f,0x77,0x5f,0x6d,0x61,0x6a,0x6f,
    0x72,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x78,0x34,0x20,0x5f,0x32,0x32,0x5f,0x6d,
    0x76,0x70,0x20,0x3a,0x20,0x70,0x61,0x63,0x6b,0x6f,0x66,0x66,0x73,0x65,0x74,0x28,
    0x63,0x30,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,
    0x5f,0x32,0x32,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x20,0x3a,0x20,0x70,0x61,0x63,0x6b,
    0x6f,0x66,0x66,0x73,0x65,0x74,0x28,0x63,0x34,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x32,0x32,0x5f,0x76,0x61,0x6c,0x20,0x3a,0x20,
    0x70,0x61,0x63,0x6b,0x6f,0x66,0x66,0x73,0x65,0x74,0x28,0x63,0x35,0x29,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x32,0x32,0x5f,0x63,0x65,
    0x6e,0x74,0x72,0x65,0x20,0x3a,0x20,0x70,0x61,0x63,0x6b,0x6f,0x66,0x66,0x73,0x65,
    0x74,0x28,0x63,0x35,0x2e,0x79,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x20,0x5f,0x32,0x32,0x5f,0x76,0x65,0x72,0x74,0x69,0x63,0x61,0x6c,0x20,
    0x3a,0x20,0x70,0x61,0x63,0x6b,0x6f,0x66,0x66,0x73,0x65,0x74,0x28,0x63,0x35,0x2e,
    0x7a,0x29,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,
    0x6f,0x6e,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,
    0x20,0x74,0x63,0x78,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x32,0x20,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x3b,0x0a,0x73,0x74,
    0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,
    0x72,0x56,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,
    0x20,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x3b,0x0a,0x73,0x74,
    0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x50,0x6f,0x73,0x69,
    0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x53,0x50,
    0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x49,0x6e,0x70,0x75,0x74,0x0a,
    0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x50,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,
    0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x32,0x20,0x54,0x65,
    0x78,0x43,0x6f,0x6f,0x72,0x64,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,
    0x44,0x31,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x53,
    0x50,0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x4f,0x75,0x74,0x70,0x75,
    0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,
    0x6f,0x6c,0x6f,0x72,0x56,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,
    0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x74,0x63,0x78,
    0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x31,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x20,0x3a,
    0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x32,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x20,0x3a,0x20,0x54,
    0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x33,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x34,0x20,0x67,0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x20,0x3a,0x20,0x53,0x56,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,
    0x7d,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x76,0x65,0x72,0x74,0x5f,0x6d,0x61,
    0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x74,0x63,0x78,0x20,0x3d,
    0x20,0x6d,0x61,0x64,0x28,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x2e,0x78,0x2c,
    0x20,0x31,0x2e,0x30,0x66,0x20,0x2d,0x20,0x5f,0x32,0x32,0x5f,0x76,0x65,0x72,0x74,
    0x69,0x63,0x61,0x6c,0x2c,0x20,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x2e,0x79,
    0x20,0x2a,0x20,0x5f,0x32,0x32,0x5f,0x76,0x65,0x72,0x74,0x69,0x63,0x61,0x6c,0x29,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x3d,0x20,0x5f,
    0x32,0x32,0x5f,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x69,
    0x6e,0x41,0x6d,0x74,0x20,0x3d,0x20,0x6d,0x69,0x6e,0x28,0x5f,0x32,0x32,0x5f,0x76,
    0x61,0x6c,0x2c,0x20,0x5f,0x32,0x32,0x5f,0x63,0x65,0x6e,0x74,0x72,0x65,0x29,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x20,0x3d,0x20,0x6d,0x61,
    0x78,0x28,0x5f,0x32,0x32,0x5f,0x76,0x61,0x6c,0x2c,0x20,0x5f,0x32,0x32,0x5f,0x63,
    0x65,0x6e,0x74,0x72,0x65,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x67,0x6c,0x5f,0x50,
    0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x6d,0x75,0x6c,0x28,0x50,0x6f,
    0x73,0x69,0x74,0x69,0x6f,0x6e,0x2c,0x20,0x5f,0x32,0x32,0x5f,0x6d,0x76,0x70,0x29,
    0x3b,0x0a,0x7d,0x0a,0x0a,0x53,0x50,0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,
    0x5f,0x4f,0x75,0x74,0x70,0x75,0x74,0x20,0x6d,0x61,0x69,0x6e,0x28,0x53,0x50,0x49,
    0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x49,0x6e,0x70,0x75,0x74,0x20,0x73,
    0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,0x29,0x0a,0x7b,0x0a,0x20,0x20,
    0x20,0x20,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x20,0x3d,0x20,0x73,0x74,0x61,
    0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,0x2e,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,
    0x64,0x3b,0x0a,0x20,0x20,0x20,0x20,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,
    0x3d,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,0x2e,0x50,0x6f,
    0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x76,0x65,0x72,0x74,
    0x5f,0x6d,0x61,0x69,0x6e,0x28,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x53,0x50,0x49,
    0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x4f,0x75,0x74,0x70,0x75,0x74,0x20,
    0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,0x67,
    0x6c,0x5f,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x67,0x6c,0x5f,
    0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,0x20,0x20,0x20,0x73,0x74,
    0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,0x74,0x63,0x78,0x20,0x3d,
    0x20,0x74,0x63,0x78,0x3b,0x0a,0x20,0x20,0x20,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,
    0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x3d,0x20,
    0x63,0x6f,0x6c,0x6f,0x72,0x56,0x3b,0x0a,0x20,0x20,0x20,0x20,0x73,0x74,0x61,0x67,
    0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x20,
    0x3d,0x20,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x20,0x20,0x20,0x20,0x73,0x74,
    0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,0x6d,0x61,0x78,0x41,0x6d,
    0x74,0x20,0x3d,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,
    0x70,0x75,0x74,0x3b,0x0a,0x7d,0x0a,0x00,
};
/*
    cbuffer fragParams : register(b0)
    {
        float4 _30_bgColor : packoffset(c0);
    };


    static float minAmt;
    static float tcx;
    static float maxAmt;
    static float4 fragColor;
    static float4 colorV;

    struct SPIRV_Cross_Input
    {
        float4 colorV : TEXCOORD0;
        float tcx : TEXCOORD1;
        float minAmt : TEXCOORD2;
        float maxAmt : TEXCOORD3;
    };

    struct SPIRV_Cross_Output
    {
        float4 fragColor : SV_Target0;
    };

    void frag_main()
    {
        float _19 = step(minAmt, tcx) - step(maxAmt, tcx);
        fragColor = (colorV * _19) + (_30_bgColor * (1.0f - _19));
    }

    SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
    {
        minAmt = stage_input.minAmt;
        tcx = stage_input.tcx;
        maxAmt = stage_input.maxAmt;
        colorV = stage_input.colorV;
        frag_main();
        SPIRV_Cross_Output stage_output;
        stage_output.fragColor = fragColor;
        return stage_output;
    }
*/
static const uint8_t fs_source_hlsl5[851] = {
    0x63,0x62,0x75,0x66,0x66,0x65,0x72,0x20,0x66,0x72,0x61,0x67,0x50,0x61,0x72,0x61,
    0x6d,0x73,0x20,0x3a,0x20,0x72,0x65,0x67,0x69,0x73,0x74,0x65,0x72,0x28,0x62,0x30,
    0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x5f,
    0x33,0x30,0x5f,0x62,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x3a,0x20,0x70,0x61,0x63,
    0x6b,0x6f,0x66,0x66,0x73,0x65,0x74,0x28,0x63,0x30,0x29,0x3b,0x0a,0x7d,0x3b,0x0a,
    0x0a,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,
    0x69,0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x20,0x74,0x63,0x78,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,0x3b,0x0a,0x73,0x74,
    0x61,0x74,0x69,0x63,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x66,0x72,0x61,0x67,
    0x43,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x73,0x74,0x61,0x74,0x69,0x63,0x20,0x66,0x6c,
    0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x3b,0x0a,0x0a,0x73,0x74,
    0x72,0x75,0x63,0x74,0x20,0x53,0x50,0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,
    0x5f,0x49,0x6e,0x70,0x75,0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x3a,0x20,0x54,0x45,0x58,
    0x43,0x4f,0x4f,0x52,0x44,0x30,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,
    0x74,0x20,0x74,0x63,0x78,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,
    0x31,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x69,0x6e,
    0x41,0x6d,0x74,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x32,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,
    0x74,0x20,0x3a,0x20,0x54,0x45,0x58,0x43,0x4f,0x4f,0x52,0x44,0x33,0x3b,0x0a,0x7d,
    0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x53,0x50,0x49,0x52,0x56,0x5f,
    0x43,0x72,0x6f,0x73,0x73,0x5f,0x4f,0x75,0x74,0x70,0x75,0x74,0x0a,0x7b,0x0a,0x20,
    0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,
    0x6c,0x6f,0x72,0x20,0x3a,0x20,0x53,0x56,0x5f,0x54,0x61,0x72,0x67,0x65,0x74,0x30,
    0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x76,0x6f,0x69,0x64,0x20,0x66,0x72,0x61,0x67,0x5f,
    0x6d,0x61,0x69,0x6e,0x28,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,
    0x61,0x74,0x20,0x5f,0x31,0x39,0x20,0x3d,0x20,0x73,0x74,0x65,0x70,0x28,0x6d,0x69,
    0x6e,0x41,0x6d,0x74,0x2c,0x20,0x74,0x63,0x78,0x29,0x20,0x2d,0x20,0x73,0x74,0x65,
    0x70,0x28,0x6d,0x61,0x78,0x41,0x6d,0x74,0x2c,0x20,0x74,0x63,0x78,0x29,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,
    0x28,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x2a,0x20,0x5f,0x31,0x39,0x29,0x20,0x2b,
    0x20,0x28,0x5f,0x33,0x30,0x5f,0x62,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x2a,0x20,
    0x28,0x31,0x2e,0x30,0x66,0x20,0x2d,0x20,0x5f,0x31,0x39,0x29,0x29,0x3b,0x0a,0x7d,
    0x0a,0x0a,0x53,0x50,0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x4f,0x75,
    0x74,0x70,0x75,0x74,0x20,0x6d,0x61,0x69,0x6e,0x28,0x53,0x50,0x49,0x52,0x56,0x5f,
    0x43,0x72,0x6f,0x73,0x73,0x5f,0x49,0x6e,0x70,0x75,0x74,0x20,0x73,0x74,0x61,0x67,
    0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x6d,
    0x69,0x6e,0x41,0x6d,0x74,0x20,0x3d,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,
    0x70,0x75,0x74,0x2e,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x3b,0x0a,0x20,0x20,0x20,0x20,
    0x74,0x63,0x78,0x20,0x3d,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,
    0x74,0x2e,0x74,0x63,0x78,0x3b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x78,0x41,0x6d,
    0x74,0x20,0x3d,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,0x2e,
    0x6d,0x61,0x78,0x41,0x6d,0x74,0x3b,0x0a,0x20,0x20,0x20,0x20,0x63,0x6f,0x6c,0x6f,
    0x72,0x56,0x20,0x3d,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x70,0x75,0x74,
    0x2e,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x72,0x61,
    0x67,0x5f,0x6d,0x61,0x69,0x6e,0x28,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x53,0x50,
    0x49,0x52,0x56,0x5f,0x43,0x72,0x6f,0x73,0x73,0x5f,0x4f,0x75,0x74,0x70,0x75,0x74,
    0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x2e,
    0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x3d,0x20,0x66,0x72,0x61,0x67,
    0x43,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,
    0x6e,0x20,0x73,0x74,0x61,0x67,0x65,0x5f,0x6f,0x75,0x74,0x70,0x75,0x74,0x3b,0x0a,
    0x7d,0x0a,0x00,
};
/*
    #include <metal_stdlib>
    #include <simd/simd.h>

    using namespace metal;

    struct vertParams
    {
        float4x4 mvp;
        float4 color;
        float val;
        float centre;
        float vertical;
    };

    struct main0_out
    {
        float4 colorV [[user(locn0)]];
        float tcx [[user(locn1)]];
        float minAmt [[user(locn2)]];
        float maxAmt [[user(locn3)]];
        float4 gl_Position [[position]];
    };

    struct main0_in
    {
        float4 Position [[attribute(0)]];
        float2 TexCoord [[attribute(1)]];
    };

    vertex main0_out main0(main0_in in [[stage_in]], constant vertParams& _22 [[buffer(0)]])
    {
        main0_out out = {};
        out.tcx = fma(in.TexCoord.x, 1.0 - _22.vertical, in.TexCoord.y * _22.vertical);
        out.colorV = _22.color;
        out.minAmt = fast::min(_22.val, _22.centre);
        out.maxAmt = fast::max(_22.val, _22.centre);
        out.gl_Position = _22.mvp * in.Position;
        return out;
    }

*/
static const uint8_t vs_source_metal_macos[867] = {
    0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,0x20,0x3c,0x6d,0x65,0x74,0x61,0x6c,0x5f,
    0x73,0x74,0x64,0x6c,0x69,0x62,0x3e,0x0a,0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,
    0x20,0x3c,0x73,0x69,0x6d,0x64,0x2f,0x73,0x69,0x6d,0x64,0x2e,0x68,0x3e,0x0a,0x0a,
    0x75,0x73,0x69,0x6e,0x67,0x20,0x6e,0x61,0x6d,0x65,0x73,0x70,0x61,0x63,0x65,0x20,
    0x6d,0x65,0x74,0x61,0x6c,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x76,
    0x65,0x72,0x74,0x50,0x61,0x72,0x61,0x6d,0x73,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x34,0x78,0x34,0x20,0x6d,0x76,0x70,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x76,0x61,0x6c,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x63,0x65,0x6e,0x74,0x72,0x65,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x76,0x65,0x72,0x74,0x69,
    0x63,0x61,0x6c,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,
    0x6d,0x61,0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,0x72,0x56,0x20,0x5b,0x5b,
    0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x30,0x29,0x5d,0x5d,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x74,0x63,0x78,0x20,0x5b,0x5b,0x75,
    0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x31,0x29,0x5d,0x5d,0x3b,0x0a,0x20,0x20,
    0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x20,0x5b,
    0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x32,0x29,0x5d,0x5d,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,0x78,0x41,0x6d,0x74,
    0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x33,0x29,0x5d,0x5d,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x67,0x6c,0x5f,
    0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x20,0x5b,0x5b,0x70,0x6f,0x73,0x69,0x74,
    0x69,0x6f,0x6e,0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,
    0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x69,0x6e,0x0a,0x7b,0x0a,0x20,0x20,0x20,
    0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,
    0x20,0x5b,0x5b,0x61,0x74,0x74,0x72,0x69,0x62,0x75,0x74,0x65,0x28,0x30,0x29,0x5d,
    0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x32,0x20,0x54,0x65,
    0x78,0x43,0x6f,0x6f,0x72,0x64,0x20,0x5b,0x5b,0x61,0x74,0x74,0x72,0x69,0x62,0x75,
    0x74,0x65,0x28,0x31,0x29,0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x76,0x65,0x72,
    0x74,0x65,0x78,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x20,0x6d,0x61,
    0x69,0x6e,0x30,0x28,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x69,0x6e,0x20,0x69,0x6e,0x20,
    0x5b,0x5b,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x5d,0x5d,0x2c,0x20,0x63,0x6f,
    0x6e,0x73,0x74,0x61,0x6e,0x74,0x20,0x76,0x65,0x72,0x74,0x50,0x61,0x72,0x61,0x6d,
    0x73,0x26,0x20,0x5f,0x32,0x32,0x20,0x5b,0x5b,0x62,0x75,0x66,0x66,0x65,0x72,0x28,
    0x30,0x29,0x5d,0x5d,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x69,0x6e,
    0x30,0x5f,0x6f,0x75,0x74,0x20,0x6f,0x75,0x74,0x20,0x3d,0x20,0x7b,0x7d,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x74,0x63,0x78,0x20,0x3d,0x20,0x66,0x6d,
    0x61,0x28,0x69,0x6e,0x2e,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,0x2e,0x78,0x2c,
    0x20,0x31,0x2e,0x30,0x20,0x2d,0x20,0x5f,0x32,0x32,0x2e,0x76,0x65,0x72,0x74,0x69,
    0x63,0x61,0x6c,0x2c,0x20,0x69,0x6e,0x2e,0x54,0x65,0x78,0x43,0x6f,0x6f,0x72,0x64,
    0x2e,0x79,0x20,0x2a,0x20,0x5f,0x32,0x32,0x2e,0x76,0x65,0x72,0x74,0x69,0x63,0x61,
    0x6c,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x63,0x6f,0x6c,0x6f,
    0x72,0x56,0x20,0x3d,0x20,0x5f,0x32,0x32,0x2e,0x63,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,
    0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x20,0x3d,
    0x20,0x66,0x61,0x73,0x74,0x3a,0x3a,0x6d,0x69,0x6e,0x28,0x5f,0x32,0x32,0x2e,0x76,
    0x61,0x6c,0x2c,0x20,0x5f,0x32,0x32,0x2e,0x63,0x65,0x6e,0x74,0x72,0x65,0x29,0x3b,
    0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x6d,0x61,0x78,0x41,0x6d,0x74,0x20,
    0x3d,0x20,0x66,0x61,0x73,0x74,0x3a,0x3a,0x6d,0x61,0x78,0x28,0x5f,0x32,0x32,0x2e,
    0x76,0x61,0x6c,0x2c,0x20,0x5f,0x32,0x32,0x2e,0x63,0x65,0x6e,0x74,0x72,0x65,0x29,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x67,0x6c,0x5f,0x50,0x6f,0x73,
    0x69,0x74,0x69,0x6f,0x6e,0x20,0x3d,0x20,0x5f,0x32,0x32,0x2e,0x6d,0x76,0x70,0x20,
    0x2a,0x20,0x69,0x6e,0x2e,0x50,0x6f,0x73,0x69,0x74,0x69,0x6f,0x6e,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,0x6e,0x20,0x6f,0x75,0x74,0x3b,0x0a,0x7d,
    0x0a,0x0a,0x00,
};
/*
    #include <metal_stdlib>
    #include <simd/simd.h>

    using namespace metal;

    struct fragParams
    {
        float4 bgColor;
    };

    struct main0_out
    {
        float4 fragColor [[color(0)]];
    };

    struct main0_in
    {
        float4 colorV [[user(locn0)]];
        float tcx [[user(locn1)]];
        float minAmt [[user(locn2)]];
        float maxAmt [[user(locn3)]];
    };

    fragment main0_out main0(main0_in in [[stage_in]], constant fragParams& _30 [[buffer(0)]])
    {
        main0_out out = {};
        float _19 = step(in.minAmt, in.tcx) - step(in.maxAmt, in.tcx);
        out.fragColor = (in.colorV * _19) + (_30.bgColor * (1.0 - _19));
        return out;
    }

*/
static const uint8_t fs_source_metal_macos[603] = {
    0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,0x20,0x3c,0x6d,0x65,0x74,0x61,0x6c,0x5f,
    0x73,0x74,0x64,0x6c,0x69,0x62,0x3e,0x0a,0x23,0x69,0x6e,0x63,0x6c,0x75,0x64,0x65,
    0x20,0x3c,0x73,0x69,0x6d,0x64,0x2f,0x73,0x69,0x6d,0x64,0x2e,0x68,0x3e,0x0a,0x0a,
    0x75,0x73,0x69,0x6e,0x67,0x20,0x6e,0x61,0x6d,0x65,0x73,0x70,0x61,0x63,0x65,0x20,
    0x6d,0x65,0x74,0x61,0x6c,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x66,
    0x72,0x61,0x67,0x50,0x61,0x72,0x61,0x6d,0x73,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,
    0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x62,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x3b,0x0a,
    0x7d,0x3b,0x0a,0x0a,0x73,0x74,0x72,0x75,0x63,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,
    0x5f,0x6f,0x75,0x74,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,
    0x34,0x20,0x66,0x72,0x61,0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x5b,0x5b,0x63,0x6f,
    0x6c,0x6f,0x72,0x28,0x30,0x29,0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x73,0x74,
    0x72,0x75,0x63,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x69,0x6e,0x0a,0x7b,0x0a,
    0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x34,0x20,0x63,0x6f,0x6c,0x6f,0x72,
    0x56,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x30,0x29,0x5d,
    0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x74,0x63,0x78,
    0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x31,0x29,0x5d,0x5d,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x69,0x6e,0x41,
    0x6d,0x74,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,0x32,0x29,
    0x5d,0x5d,0x3b,0x0a,0x20,0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x6d,0x61,
    0x78,0x41,0x6d,0x74,0x20,0x5b,0x5b,0x75,0x73,0x65,0x72,0x28,0x6c,0x6f,0x63,0x6e,
    0x33,0x29,0x5d,0x5d,0x3b,0x0a,0x7d,0x3b,0x0a,0x0a,0x66,0x72,0x61,0x67,0x6d,0x65,
    0x6e,0x74,0x20,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x6f,0x75,0x74,0x20,0x6d,0x61,0x69,
    0x6e,0x30,0x28,0x6d,0x61,0x69,0x6e,0x30,0x5f,0x69,0x6e,0x20,0x69,0x6e,0x20,0x5b,
    0x5b,0x73,0x74,0x61,0x67,0x65,0x5f,0x69,0x6e,0x5d,0x5d,0x2c,0x20,0x63,0x6f,0x6e,
    0x73,0x74,0x61,0x6e,0x74,0x20,0x66,0x72,0x61,0x67,0x50,0x61,0x72,0x61,0x6d,0x73,
    0x26,0x20,0x5f,0x33,0x30,0x20,0x5b,0x5b,0x62,0x75,0x66,0x66,0x65,0x72,0x28,0x30,
    0x29,0x5d,0x5d,0x29,0x0a,0x7b,0x0a,0x20,0x20,0x20,0x20,0x6d,0x61,0x69,0x6e,0x30,
    0x5f,0x6f,0x75,0x74,0x20,0x6f,0x75,0x74,0x20,0x3d,0x20,0x7b,0x7d,0x3b,0x0a,0x20,
    0x20,0x20,0x20,0x66,0x6c,0x6f,0x61,0x74,0x20,0x5f,0x31,0x39,0x20,0x3d,0x20,0x73,
    0x74,0x65,0x70,0x28,0x69,0x6e,0x2e,0x6d,0x69,0x6e,0x41,0x6d,0x74,0x2c,0x20,0x69,
    0x6e,0x2e,0x74,0x63,0x78,0x29,0x20,0x2d,0x20,0x73,0x74,0x65,0x70,0x28,0x69,0x6e,
    0x2e,0x6d,0x61,0x78,0x41,0x6d,0x74,0x2c,0x20,0x69,0x6e,0x2e,0x74,0x63,0x78,0x29,
    0x3b,0x0a,0x20,0x20,0x20,0x20,0x6f,0x75,0x74,0x2e,0x66,0x72,0x61,0x67,0x43,0x6f,
    0x6c,0x6f,0x72,0x20,0x3d,0x20,0x28,0x69,0x6e,0x2e,0x63,0x6f,0x6c,0x6f,0x72,0x56,
    0x20,0x2a,0x20,0x5f,0x31,0x39,0x29,0x20,0x2b,0x20,0x28,0x5f,0x33,0x30,0x2e,0x62,
    0x67,0x43,0x6f,0x6c,0x6f,0x72,0x20,0x2a,0x20,0x28,0x31,0x2e,0x30,0x20,0x2d,0x20,
    0x5f,0x31,0x39,0x29,0x29,0x3b,0x0a,0x20,0x20,0x20,0x20,0x72,0x65,0x74,0x75,0x72,
    0x6e,0x20,0x6f,0x75,0x74,0x3b,0x0a,0x7d,0x0a,0x0a,0x00,
};
static inline const sg_shader_desc* slider_shader_desc(sg_backend backend) {
    if (backend == SG_BACKEND_GLES3) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.attrs[0].name = "Position";
            desc.attrs[1].name = "TexCoord";
            desc.vs.source = (const char*)vs_source_glsl300es;
            desc.vs.entry = "main";
            desc.vs.uniform_blocks[0].size = 96;
            desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.vs.uniform_blocks[0].uniforms[0].name = "vertParams";
            desc.vs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
            desc.vs.uniform_blocks[0].uniforms[0].array_count = 6;
            desc.fs.source = (const char*)fs_source_glsl300es;
            desc.fs.entry = "main";
            desc.fs.uniform_blocks[0].size = 16;
            desc.fs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.fs.uniform_blocks[0].uniforms[0].name = "fragParams";
            desc.fs.uniform_blocks[0].uniforms[0].type = SG_UNIFORMTYPE_FLOAT4;
            desc.fs.uniform_blocks[0].uniforms[0].array_count = 1;
            desc.label = "slider_shader";
        }
        return &desc;
    }
    if (backend == SG_BACKEND_D3D11) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.attrs[0].sem_name = "TEXCOORD";
            desc.attrs[0].sem_index = 0;
            desc.attrs[1].sem_name = "TEXCOORD";
            desc.attrs[1].sem_index = 1;
            desc.vs.source = (const char*)vs_source_hlsl5;
            desc.vs.d3d11_target = "vs_5_0";
            desc.vs.entry = "main";
            desc.vs.uniform_blocks[0].size = 96;
            desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.fs.source = (const char*)fs_source_hlsl5;
            desc.fs.d3d11_target = "ps_5_0";
            desc.fs.entry = "main";
            desc.fs.uniform_blocks[0].size = 16;
            desc.fs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.label = "slider_shader";
        }
        return &desc;
    }
    if (backend == SG_BACKEND_METAL_MACOS) {
        static sg_shader_desc desc;
        static bool valid;
        if (!valid) {
            valid = true;
            desc.vs.source = (const char*)vs_source_metal_macos;
            desc.vs.entry = "main0";
            desc.vs.uniform_blocks[0].size = 96;
            desc.vs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.fs.source = (const char*)fs_source_metal_macos;
            desc.fs.entry = "main0";
            desc.fs.uniform_blocks[0].size = 16;
            desc.fs.uniform_blocks[0].layout = SG_UNIFORMLAYOUT_STD140;
            desc.label = "slider_shader";
        }
        return &desc;
    }
    return 0;
}
static inline int slider_attr_slot(const char* attr_name) {
    (void)attr_name;
    if (0 == strcmp(attr_name, "Position")) {
        return 0;
    }
    if (0 == strcmp(attr_name, "TexCoord")) {
        return 1;
    }
    return -1;
}
static inline int slider_image_slot(sg_shader_stage stage, const char* img_name) {
    (void)stage; (void)img_name;
    return -1;
}
static inline int slider_sampler_slot(sg_shader_stage stage, const char* smp_name) {
    (void)stage; (void)smp_name;
    return -1;
}
static inline int slider_uniformblock_slot(sg_shader_stage stage, const char* ub_name) {
    (void)stage; (void)ub_name;
    if (SG_SHADERSTAGE_VS == stage) {
        if (0 == strcmp(ub_name, "vertParams")) {
            return 0;
        }
    }
    if (SG_SHADERSTAGE_FS == stage) {
        if (0 == strcmp(ub_name, "fragParams")) {
            return 0;
        }
    }
    return -1;
}
static inline size_t slider_uniformblock_size(sg_shader_stage stage, const char* ub_name) {
    (void)stage; (void)ub_name;
    if (SG_SHADERSTAGE_VS == stage) {
        if (0 == strcmp(ub_name, "vertParams")) {
            return sizeof(vertParams_t);
        }
    }
    if (SG_SHADERSTAGE_FS == stage) {
        if (0 == strcmp(ub_name, "fragParams")) {
            return sizeof(fragParams_t);
        }
    }
    return 0;
}
static inline int slider_uniform_offset(sg_shader_stage stage, const char* ub_name, const char* u_name) {
    (void)stage; (void)ub_name; (void)u_name;
    if (SG_SHADERSTAGE_VS == stage) {
        if (0 == strcmp(ub_name, "vertParams")) {
            if (0 == strcmp(u_name, "mvp")) {
                return 0;
            }
            if (0 == strcmp(u_name, "color")) {
                return 64;
            }
            if (0 == strcmp(u_name, "val")) {
                return 80;
            }
            if (0 == strcmp(u_name, "centre")) {
                return 84;
            }
            if (0 == strcmp(u_name, "vertical")) {
                return 88;
            }
        }
    }
    if (SG_SHADERSTAGE_FS == stage) {
        if (0 == strcmp(ub_name, "fragParams")) {
            if (0 == strcmp(u_name, "bgColor")) {
                return 0;
            }
        }
    }
    return -1;
}
static inline sg_shader_uniform_desc slider_uniform_desc(sg_shader_stage stage, const char* ub_name, const char* u_name) {
    (void)stage; (void)ub_name; (void)u_name;
    #if defined(__cplusplus)
    sg_shader_uniform_desc desc = {};
    #else
    sg_shader_uniform_desc desc = {0};
    #endif
    if (SG_SHADERSTAGE_VS == stage) {
        if (0 == strcmp(ub_name, "vertParams")) {
            if (0 == strcmp(u_name, "mvp")) {
                desc.name = "mvp";
                desc.type = SG_UNIFORMTYPE_MAT4;
                desc.array_count = 0;
                return desc;
            }
            if (0 == strcmp(u_name, "color")) {
                desc.name = "color";
                desc.type = SG_UNIFORMTYPE_FLOAT4;
                desc.array_count = 0;
                return desc;
            }
            if (0 == strcmp(u_name, "val")) {
                desc.name = "val";
                desc.type = SG_UNIFORMTYPE_FLOAT;
                desc.array_count = 0;
                return desc;
            }
            if (0 == strcmp(u_name, "centre")) {
                desc.name = "centre";
                desc.type = SG_UNIFORMTYPE_FLOAT;
                desc.array_count = 0;
                return desc;
            }
            if (0 == strcmp(u_name, "vertical")) {
                desc.name = "vertical";
                desc.type = SG_UNIFORMTYPE_FLOAT;
                desc.array_count = 0;
                return desc;
            }
        }
    }
    if (SG_SHADERSTAGE_FS == stage) {
        if (0 == strcmp(ub_name, "fragParams")) {
            if (0 == strcmp(u_name, "bgColor")) {
                desc.name = "bgColor";
                desc.type = SG_UNIFORMTYPE_FLOAT4;
                desc.array_count = 0;
                return desc;
            }
        }
    }
    return desc;
}
static inline int slider_storagebuffer_slot(sg_shader_stage stage, const char* sbuf_name) {
    (void)stage; (void)sbuf_name;
    return -1;
}
