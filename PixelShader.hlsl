#include "Lighting.hlsli"
#include "ShaderIncludes.hlsli"

#define MAX_LIGHTS 128

cbuffer ExternalData : register (b0)
{
	Light lights[MAX_LIGHTS];
	int lightCount;

	float3 cameraPos;
	int renderShadows;
}

Texture2D Albedo		: register(t0);
Texture2D NormalMap		: register(t1);
Texture2D RoughnessMap	: register(t2);
Texture2D MetalnessMap	: register(t3);
Texture2D RampMap		: register(t4);
Texture2D shadowMap		: register(t5);
Texture2D specularRampMap		: register(t6);
SamplerState SamplerOptions	: register(s0);
SamplerState ClampSampler	: register(s1);
SamplerComparisonState shadowSampler	: register(s2);

struct PSOutput 
{
	float4 color		: SV_TARGET0;
	float4 normals		: SV_TARGET1;
	float4 depth		: SV_TARGET2;
};

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
PSOutput main(VertexToPixelNormalShadowMap input)
{
	// Just return the input color
	// - This color (like most values passing through the rasterizer) is 
	//   interpolated for each pixel between the corresponding vertices 
	//   of the triangle we're rendering

	// normalize the normal and tangent
	float3 normal = normalize(input.normal);
	float3 tangent = normalize(input.tangent);
	tangent = normalize(tangent - normal * dot(tangent, normal)); // Gram-Schmidt orthoganlization

	float3 biTangent = cross(tangent, normal);
	float3x3 TBN = float3x3(tangent, biTangent, normal);
	 
	// get the surface color - gamme correct
	float4 surfaceColor = Albedo.Sample(SamplerOptions, input.uv);
	surfaceColor.rgb = pow(surfaceColor.rgb, 2.2);

	// get the normal color
	float3 unpackedNormal = NormalMap.Sample(SamplerOptions, input.uv).rgb * 2 - 1;

	// normalize the normal after applying TBN matrix
	normal = mul(unpackedNormal, TBN);
	normal = normalize(normal);

	// get the metalness
	float metalness = MetalnessMap.Sample(SamplerOptions, input.uv).r;

	// get the roughness
	float roughness = RoughnessMap.Sample(SamplerOptions, input.uv).r;

	// get the specular color
	// Specular color determination -----------------
	// Assume albedo texture is actually holding specular color where metalness == 1
	//
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we want lerp the specular color to match 
	float3 specularColor = lerp(F0_NON_METAL.rrr, surfaceColor.rgb, metalness);

	float3 totalColor = float3(0, 0, 0);
	for (int i = 0; i < lightCount; i++)
	{
		if (0 == lights[i].Enabled)
			continue;

		switch (lights[i].Type) 
		{
		case TYPE_DIRECTIONAL:
			totalColor += dLightColorPBRToon(normal, input.worldPos, lights[i], cameraPos, metalness, roughness, surfaceColor.rgb, specularColor, RampMap, specularRampMap, ClampSampler);
			break;
		case TYPE_POINT:
			totalColor += pLightColorPBRToon(normal, input.worldPos, lights[i], cameraPos, metalness, roughness, surfaceColor.rgb, specularColor, RampMap, specularRampMap, ClampSampler);
			break;
		case TYPE_SPOT:
			totalColor += sLightColorPBRToon(normal, input.worldPos, lights[i], cameraPos, metalness, roughness, surfaceColor.rgb, specularColor, RampMap, specularRampMap, ClampSampler);
			break;		
		}
	}

	// Shadow Mapping
	// Calculate this pixel's UV coord on the shadow map
	if (renderShadows != 0)
	{
		// Convert from homogeneous screen coords to UV coords
		// remembering to flip the y value
		float2 shadowUV = input.posForShadows.xy / input.posForShadows.w * 0.5f + 0.5f;
		shadowUV.y = 1.0f - shadowUV.y;

		// Calculate this pixel's depth from the light
		float depthFromLight = input.posForShadows.z / input.posForShadows.w;

		// Use a comparison sampler to compare the results of a 2x2 group of neighboring pixels
		// and return the ratio of how many "passed" the comparison
		float shadowAmount = shadowMap.SampleCmpLevelZero(shadowSampler, shadowUV, depthFromLight);
		totalColor *= shadowAmount;
	}

	// generate output
	PSOutput output;
	
	output.color = float4(pow(totalColor, 1.0f / 2.2f), 1.0f);
	output.normals = float4(input.normal, 0);
	output.depth = 0.5f;

	// calculate and return final color
	return output;
}