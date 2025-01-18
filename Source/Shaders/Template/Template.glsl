#ifndef TEMPLATE_GLSL
#define TEMPLATE_GLSL

#extension GL_GOOGLE_include_directive : require

#ifndef SHADER_STAGE
    #include "Common/Stages.h"
    #define SHADER_STAGE COMPUTE_STAGE
    #pragma shader_stage(compute)
    void main() {}
#endif

#include "Common/Common.h"
#include "Common/Common.glsl"
#include "Common/Constants.glsl"

#ifndef SHADER_LAYOUT
    #include "Template/Template.layout"
#endif

#if SOME_DEFINE
    vec4 GetSmth()
    {
        return smth;
    }
#endif

vec4 ComputeSmth(vec4 argument)
{
    return global + frame + object + argument;
}

#endif
