#include "ShaderIncludes.hlsli"

cbuffer ExternalData : register (b0)
{
	float4 colorTint;
	matrix world;
	matrix view;
	matrix projection;
	matrix invTransposeWorld;
	matrix shadowView;
	matrix shadowProjection;
}

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// 
// - Input is exactly one vertex worth of data (defined by a struct)
// - Output is a single struct of data to pass down the pipeline
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
VertexToPixelNormalShadowMap main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixelNormalShadowMap output;
	output.uv = input.uv;

	// Here we're essentially passing the input position directly through to the next
	// stage (rasterizer), though it needs to be a 4-component vector now.  
	// - To be considered within the bounds of the screen, the X and Y components 
	//   must be between -1 and 1.  
	// - The Z component must be between 0 and 1.  
	// - Each of these components is then automatically divided by the W component, 
	//   which we're leaving at 1.0 for now (this is more useful when dealing with 
	//   a perspective projection matrix, which we'll get to in the future).
	matrix wvp = mul(projection, mul(view, world));
	output.position = mul(wvp, float4(input.position, 1.0f));

	// figure out where vertex is in shadow map
	matrix shadowWVP = mul(shadowProjection, mul(shadowView, world));
	output.posForShadows = mul(shadowWVP, float4(input.position, 1.0f));

	// Pass the color through 
	// - The values will be interpolated per-pixel by the rasterizer
	// - We don't need to alter it here, but we do need to send it to the pixel shader
	output.color = colorTint;

	// use inverse transpose world matrix to account for non-uniform scaling
	output.normal = normalize(mul((float3x3)invTransposeWorld, input.normal));

	// send world position of vertex for point/spot lights
	output.worldPos = mul(world, float4(input.position, 1.0f)).xyz;

	// use inverse transpose world matric to account for non-uniform scaling
	output.tangent = normalize(mul((float3x3)invTransposeWorld, input.tangent));

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}