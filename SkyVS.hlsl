#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register (b0)
{
	matrix view;
	matrix projection;
}

struct VertexToPixel_Sky
{
	float4 position		:	SV_POSITION;
	float3 sampleDir	:	DIRECTION;
};

VertexToPixel_Sky main( VertexShaderInput input )
{
	// create output struct
	VertexToPixel_Sky output;

	// 0 the translation of view matrix
	matrix viewNoTranslation = view;
	viewNoTranslation._14 = 0;
	viewNoTranslation._24 = 0;
	viewNoTranslation._34 = 0;

	// apply view and proj to position, set to output position
	// then set the output z to to the w to ensure the depth is 1.0
	matrix viewProjection = mul(projection, viewNoTranslation);
	output.position = mul(viewProjection, float4(input.position, 1.0f));
	output.position.z = output.position.w;

	output.sampleDir = input.position;

	return output;
}