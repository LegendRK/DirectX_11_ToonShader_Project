// Vertex Shader to be used when rendering TO the shadow map

cbuffer ExternalData : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
}

// Struct representing a single vertex worth of data
struct VertexShaderInput
{
	float3 position		: POSITION;     // XYZ position
	float2 uv			: TEXCOORD;
	float3 normal		: NORMAL;
	float3 tangent		: TANGENT;
};

// Struct representing the data we're sending down the pipeline
struct VertexToPixel
{
	float4 position		: SV_POSITION;	// XYZW position (System Value Position)
};

// --------------------------------------------------------
// The entry point (main method) for our vertex shader
// --------------------------------------------------------
VertexToPixel main(VertexShaderInput input)
{
	// Set up output struct
	VertexToPixel output;

	// Modifying the position using the provided transformation (world) matrix
	matrix wvp = mul(projection, mul(view, world));
	output.position = mul(wvp, float4(input.position, 1.0f));

	// Whatever we return will make its way through the pipeline to the
	// next programmable stage we're using (the pixel shader for now)
	return output;
}