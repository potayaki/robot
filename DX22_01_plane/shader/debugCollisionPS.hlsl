#include "common.hlsl"

// No texture or sampler: a previously bound dice/model texture cannot affect these lines.
float4 main(PS_IN input) : SV_Target
{
    return input.col;
}
