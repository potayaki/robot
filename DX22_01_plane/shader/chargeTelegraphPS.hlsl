#include "common.hlsl"

float4 main(PS_IN input) : SV_Target
{
    return input.col; // No texture or previously bound material affects this warning.
}
