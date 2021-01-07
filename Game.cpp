#include "Game.h"
#include "BufferStructs.h"

// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",	   // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif

	pixelShader = 0;
	vertexShader = 0;
	skyPixelShader = 0;
	skyVertexShader = 0;

	ppVS = 0;
	ppPS = 0;

	controlMode = 0;
	prevTab = false;
	prevV = false;
	dirLightDirection = XMFLOAT3(0.0f, -1.0f, 0.0f);
	pointLightPosition = XMFLOAT3(0.0f, 5.0f, 0.0f);
	pointLightRange = 20.0f;
	spotLightDirection = XMFLOAT3(0.0f, 0.0f, -1.0f);
	spotLightPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	spotLightRange = 10.0f;
	spotLightFalloff = 25.0f;

	shadowMapSize = 0;
	shadowViewMatrix = {};
	shadowProjectionMatrix = {};
	shadowVS = 0;
	enableShadows = true;

	mainCamera = 0;
	skyBox = 0;
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Note: Since we're using smart pointers (ComPtr),
	// we don't need to explicitly clean up those DirectX objects
	// - If we weren't using smart pointers, we'd need
	//   to call Release() on each DirectX object created in Game
	
	// cleanup dynamic memory
	while (!entities.empty()) {
		delete entities.back();
		entities.pop_back();
	}

	while (!meshes.empty()) {
		delete meshes.back();
		meshes.pop_back();
	}

	while (!materials.empty()) {
		delete materials.back();
		materials.pop_back();
	}

	if (mainCamera) { delete mainCamera; }
	if (skyBox) { delete skyBox; }
	if (pixelShader) { delete pixelShader; }
	if (vertexShader) { delete vertexShader; }
	if (skyPixelShader) { delete skyPixelShader; }
	if (skyVertexShader) { delete skyVertexShader; }
	if (ppVS) { delete ppVS; }
	if (ppPS) { delete ppPS; }
	if (shadowVS) { delete shadowVS; }
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	LoadTextures();
	CreateBasicGeometry();

	// create skyBox - can use either .dds or 6 texture method
	skyBox = new SkyBox(meshes[1], sampler, device, GetFullPathTo_Wide(L"../../Assets/Textures/SkyBox/SunnyCubeMap.dds"), skyVertexShader, skyPixelShader);

	// Tell the input assembler stage of the pipeline what kind of
	// geometric primitives (points, lines or triangles) we want to draw.  
	// Essentially: "What kind of shape should the GPU draw with our data?"
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// create cbuffer for vertex shader
	// Get size as the next multiple of 16 (don't hardcode the number here!)
	unsigned int size = sizeof(VertexShaderExternalData);
	size = (size + 15) / 16 * 16; // This will work even if your struct size changes

	// Describe the constant buffer
	D3D11_BUFFER_DESC cbDesc = {};	// Sets struct to all zeroes
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = size; // Must be a multiple of 16
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	// initialize the lights
	GenerateLights();

	// initialize the shadowmap
	InitializeShadowMap();

	// create the post processing resources
	ResizePostProcessResources();

	// Set up sprite batch and sprite font
	spriteBatch = std::make_unique<SpriteBatch>(context.Get());
	spriteFont = std::make_unique<SpriteFont>(device.Get(), GetFullPathTo_Wide(L"../../Assets/Textures/arial.spritefont").c_str());
	spriteFontLarge = std::make_unique<SpriteFont>(device.Get(), GetFullPathTo_Wide(L"../../Assets/Textures/arial72.spritefont").c_str());

	// create the camera
	mainCamera = new Camera(0.0f, -2.0f, 4.5f, (float)this->width / this->height, 0.25 * XM_PI, 0.01f, 100.0f, 6.0f, 10.0f);
	mainCamera->GetTransform()->SetPitchYawRoll(0.05f, XM_PI, 0.0f);
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	vertexShader = new SimpleVertexShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"VertexShader.cso").c_str());
	pixelShader = new SimplePixelShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"PixelShader.cso").c_str());

	skyVertexShader = new SimpleVertexShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"SkyVS.cso").c_str());
	skyPixelShader = new SimplePixelShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"SkyPS.cso").c_str());

	ppVS = new SimpleVertexShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"PostProcessVS.cso").c_str());
	ppPS = new SimplePixelShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"PostProcessPS.cso").c_str());

    shadowVS = new SimpleVertexShader(device.Get(), context.Get(), GetFullPathTo_Wide(L"ShadowMapVS.cso").c_str());
}

void Game::LoadBasicTexture(const wchar_t* file, ID3D11ShaderResourceView** textureSRV)
{
	CreateWICTextureFromFile(device.Get(), context.Get(), GetFullPathTo_Wide(file).c_str(), nullptr, textureSRV);
}

void Game::LoadPBRTexture(const wchar_t* albedoPath, ID3D11ShaderResourceView** albedoSRV, const wchar_t* normalPath, ID3D11ShaderResourceView** normalSRV, const wchar_t* metalPath, ID3D11ShaderResourceView** metalSRV, const wchar_t* roughnessPath, ID3D11ShaderResourceView** roughnessSRV)
{
	LoadBasicTexture(albedoPath, albedoSRV);
	LoadBasicTexture(normalPath, normalSRV);
	LoadBasicTexture(metalPath, metalSRV);
	LoadBasicTexture(roughnessPath, roughnessSRV);
}

void Game::LoadTextures()
{
	// load carpet texture
	LoadPBRTexture(
		L"../../Assets/Textures/carpet_albedo.png", carpetA_SRV.GetAddressOf(),
		L"../../Assets/Textures/carpet_normals.png", carpetN_SRV.GetAddressOf(),
		L"../../Assets/Textures/carpet_metal.png", carpetM_SRV.GetAddressOf(),
		L"../../Assets/Textures/carpet_roughness.png", carpetR_SRV.GetAddressOf()
	);

	// load wall texture
	LoadPBRTexture(
		L"../../Assets/Textures/wall_albedo.png", wallA_SRV.GetAddressOf(),
		L"../../Assets/Textures/wall_normals.png", wallN_SRV.GetAddressOf(),
		L"../../Assets/Textures/wall_metal.png", wallM_SRV.GetAddressOf(),
		L"../../Assets/Textures/wall_roughness.png", wallR_SRV.GetAddressOf()
	);

    // load table texture
    LoadPBRTexture(
        L"../../Assets/Textures/table_albedo.png", tableA_SRV.GetAddressOf(),
        L"../../Assets/Textures/table_normals.png", tableN_SRV.GetAddressOf(),
        L"../../Assets/Textures/table_metal.png", tableM_SRV.GetAddressOf(),
        L"../../Assets/Textures/table_roughness.png", tableR_SRV.GetAddressOf()
    );

    // load sofa texture
    LoadPBRTexture(
        L"../../Assets/Textures/sofa_albedo.jpg", sofaA_SRV.GetAddressOf(),
        L"../../Assets/Textures/sofa_normals.jpg", sofaN_SRV.GetAddressOf(),
        L"../../Assets/Textures/sofa_metal.jpg", sofaM_SRV.GetAddressOf(),
        L"../../Assets/Textures/sofa_roughness.jpg", sofaR_SRV.GetAddressOf()
    );

    // load tv texture
    LoadPBRTexture(
        L"../../Assets/Textures/tv_albedo.png", tvA_SRV.GetAddressOf(),
        L"../../Assets/Textures/tv_normals.png", tvN_SRV.GetAddressOf(),
        L"../../Assets/Textures/tv_metal.png", tvM_SRV.GetAddressOf(),
        L"../../Assets/Textures/tv_roughness.png", tvR_SRV.GetAddressOf()
    );

    // load tv stand texture
    LoadPBRTexture(
        L"../../Assets/Textures/coffeeTable_albedo.jpg", coffeeTableA_SRV.GetAddressOf(),
        L"../../Assets/Textures/coffeeTable_normals.jpg", coffeeTableN_SRV.GetAddressOf(),
        L"../../Assets/Textures/coffeeTable_metal.jpg", coffeeTableM_SRV.GetAddressOf(),
        L"../../Assets/Textures/coffeeTable_roughness.jpg", coffeeTableR_SRV.GetAddressOf()
    );

	// load newton cradle textures
	LoadPBRTexture(
		L"../../Assets/Textures/cradle_albedo.png", cradleA_SRV.GetAddressOf(),
		L"../../Assets/Textures/cradle_normals.png", cradleN_SRV.GetAddressOf(),
		L"../../Assets/Textures/cradle_metal.png", cradleM_SRV.GetAddressOf(),
		L"../../Assets/Textures/cradle_roughness.png", cradleR_SRV.GetAddressOf()
	);

	// load sword textures
	LoadPBRTexture(
		L"../../Assets/Textures/sword_albedo.png", swordA_SRV.GetAddressOf(),
		L"../../Assets/Textures/sword_normals.png", swordN_SRV.GetAddressOf(),
		L"../../Assets/Textures/sword_metal.png", swordM_SRV.GetAddressOf(),
		L"../../Assets/Textures/sword_roughness.png", swordR_SRV.GetAddressOf()
	);

	// load toon ramp textures
	LoadBasicTexture(L"../../Assets/Textures/Ramp/toonRamp.png", toonRamp_SRV.GetAddressOf());
	LoadBasicTexture(L"../../Assets/Textures/Ramp/toonRamp1.png", toonRamp1_SRV.GetAddressOf());
	LoadBasicTexture(L"../../Assets/Textures/Ramp/toonRamp2.png", toonRamp2_SRV.GetAddressOf());
	LoadBasicTexture(L"../../Assets/Textures/Ramp/toonRamp3.png", toonRamp3_SRV.GetAddressOf());
	LoadBasicTexture(L"../../Assets/Textures/Ramp/toonRampSpecular.png", specularToonRamp_SRV.GetAddressOf());
	

	D3D11_SAMPLER_DESC sDesc = {};
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	sDesc.MaxAnisotropy = 1;
	sDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sDesc, sampler.GetAddressOf());

	// Make a second sampler that's uses clamp addressing
	sDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	device->CreateSamplerState(&sDesc, clampSampler.GetAddressOf());
}

void Game::CreateBasicGeometry()
{
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/cone.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/cube.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/cylinder.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/helix.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/sphere.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/torus.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/table.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/sofa.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/tv.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/coffeeTable.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/cradle.obj").c_str(), device));
	meshes.push_back(new Mesh(GetFullPathTo("../../Assets/Models/sword.obj").c_str(), device));


	// create materials - PBR
	Material* matCarpet = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, carpetA_SRV, carpetN_SRV, carpetM_SRV, carpetR_SRV);
	Material* matWall = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, wallA_SRV, wallN_SRV, wallM_SRV, wallR_SRV);
	Material* matTable = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, tableA_SRV, tableN_SRV, tableM_SRV, tableR_SRV);
	Material* matSofa = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, sofaA_SRV, sofaN_SRV, sofaM_SRV, sofaR_SRV);
	Material* matTV = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, tvA_SRV, tvN_SRV, tvM_SRV, tvR_SRV);
	Material* matCoffeeTable = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, coffeeTableA_SRV, coffeeTableN_SRV, coffeeTableM_SRV, coffeeTableR_SRV);
	Material* matCradle = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, cradleA_SRV, cradleN_SRV, cradleM_SRV, cradleR_SRV);
	Material* matSword = new Material(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), pixelShader, vertexShader, 1.0f, sampler, swordA_SRV, swordN_SRV, swordM_SRV, swordR_SRV);
	materials.push_back(matCarpet);
	materials.push_back(matWall);
	materials.push_back(matTable);
	materials.push_back(matSofa);
	materials.push_back(matTV);
	materials.push_back(matCoffeeTable);
	materials.push_back(matCradle);
	materials.push_back(matSword);

	// make sphere entitites with PBR textures
	entities.push_back(new Entity(meshes[1], matCarpet));
	entities.push_back(new Entity(meshes[1], matWall));
	entities.push_back(new Entity(meshes[1], matWall));
	entities.push_back(new Entity(meshes[1], matWall));
	entities.push_back(new Entity(meshes[1], matWall));
	entities.push_back(new Entity(meshes[6], matTable));
	entities.push_back(new Entity(meshes[7], matSofa));
	entities.push_back(new Entity(meshes[8], matTV));
	entities.push_back(new Entity(meshes[9], matCoffeeTable));
	entities.push_back(new Entity(meshes[10], matCradle));
	entities.push_back(new Entity(meshes[11], matSword));
}

void Game::GenerateLights()
{
	// create directional lights
	Light dir1 = {};
	dir1.type = TYPE_DIRECTIONAL;
	dir1.direction = dirLightDirection;
	dir1.diffuseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
	dir1.ambientColor = XMFLOAT3(0.01f, 0.01f, 0.01f);
	dir1.intensity = 1.0f;
	dir1.enabled = 1;

	lights.push_back(dir1);

	// create point lights
	Light point1 = {};
	point1.type = TYPE_POINT;
	point1.position = pointLightPosition;
	point1.ambientColor = XMFLOAT3(0.01f, 0.01f, 0.01f);
	point1.diffuseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
	point1.radius = pointLightRange;
	point1.intensity = 1.0f;
	point1.enabled = 0;

	lights.push_back(point1);

	// create spot lights
	Light spot1 = {};
	spot1.type = TYPE_SPOT;
	spot1.position = spotLightPosition;
	spot1.ambientColor = XMFLOAT3(0.01f, 0.01f, 0.01f);
	spot1.diffuseColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
	spot1.direction = spotLightDirection;
	spot1.spotPower = spotLightFalloff;
	spot1.radius = spotLightRange;
	spot1.intensity = 10.0f;
	spot1.enabled = 0;

	lights.push_back(spot1);
}

void Game::InitializeShadowMap()
{
	// SHADOW MAP INITIALIZATION --------------------------------------

	// In general, this should be a power of 2
	shadowMapSize = 1024;

	// Create an underlying texture resource for the shadow map
	// First, describe what we want
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapSize;
	shadowDesc.Height = shadowMapSize;
	shadowDesc.ArraySize = 1; // Not an array
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // We'll bind as one or the other
	shadowDesc.CPUAccessFlags = 0; // Not reading back in C++
	shadowDesc.Format = DXGI_FORMAT_R32_TYPELESS; // All 32 bits in a single channel (number)
	shadowDesc.MipLevels = 1;
	shadowDesc.MiscFlags = 0;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.SampleDesc.Quality = 0;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;

	// Create the texture itself
	ID3D11Texture2D* shadowTexture;
	device->CreateTexture2D(&shadowDesc, 0, &shadowTexture);
	if (!shadowTexture)
		return;

	// Create the depth/stencil view for the shadow map
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Flags = 0;
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0; // Render into the first mip
	device->CreateDepthStencilView(shadowTexture, &dsvDesc, shadowDSV.GetAddressOf());

	// Create the shader resource view for the shadow map
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	device->CreateShaderResourceView(shadowTexture, &srvDesc, shadowSRV.GetAddressOf());

	// Now that we have both views, we can release the
	// reference to the texture itself!
	shadowTexture->Release();


	// Create the rasterizer state to add bias to depth values
	// when creating the shadow map each frame
	D3D11_RASTERIZER_DESC shRast = {};
	shRast.FillMode = D3D11_FILL_SOLID;
	shRast.CullMode = D3D11_CULL_BACK;
	shRast.DepthClipEnable = true;
	shRast.DepthBias = 1000; // 1000 units of precision
	shRast.DepthBiasClamp = 0.0f;
	shRast.SlopeScaledDepthBias = 1.0f;
	device->CreateRasterizerState(&shRast, shadowRasterizer.GetAddressOf());


	// Create a sampler state for sampling the shadow map with
	// different options than we use for our "regular" textures
	D3D11_SAMPLER_DESC shSamp = {};
	shSamp.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shSamp.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shSamp.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shSamp.BorderColor[0] = 1.0f;
	shSamp.BorderColor[1] = 1.0f;
	shSamp.BorderColor[2] = 1.0f;
	shSamp.BorderColor[3] = 1.0f;
	shSamp.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR; // COMPARISON filter!  For use with comparison samplers!
	shSamp.ComparisonFunc = D3D11_COMPARISON_LESS;
	device->CreateSamplerState(&shSamp, shadowSampler.GetAddressOf());


	// Update the Shadow Map View matrix
	UpdateShadowMapView();

	XMMATRIX shProj = XMMatrixOrthographicLH(
		25,			// Width of projection "box" in world units
		25,			// Height of "
		0.1f,
		30.0f);
	XMStoreFloat4x4(&shadowProjectionMatrix, shProj);
}

void Game::ResizePostProcessResources()
{
	D3D11_TEXTURE2D_DESC textureDesc = {};
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.ArraySize = 1;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; // Will render to it and sample from it!
	textureDesc.CPUAccessFlags = 0;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.MipLevels = 1;
	textureDesc.MiscFlags = 0;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;

	// Create the color and normals textures
	ID3D11Texture2D* ppTexture;
	device->CreateTexture2D(&textureDesc, 0, &ppTexture);
	if (!ppTexture)
		return;

	// Adjust the description for scene normals
	textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	ID3D11Texture2D* sceneNormalsTexture;
	device->CreateTexture2D(&textureDesc, 0, &sceneNormalsTexture);
	if (!sceneNormalsTexture)
		return;

	// Adjust the description for the scene depths
	textureDesc.Format = DXGI_FORMAT_R32_FLOAT;
	ID3D11Texture2D* sceneDepthsTexture;
	device->CreateTexture2D(&textureDesc, 0, &sceneDepthsTexture);
	if (!sceneDepthsTexture)
		return;

	// Create the Render Target Views
	device->CreateRenderTargetView(ppTexture, 0, ppRTV.ReleaseAndGetAddressOf());
	device->CreateRenderTargetView(sceneNormalsTexture, 0, sceneNormalsRTV.ReleaseAndGetAddressOf());
	device->CreateRenderTargetView(sceneDepthsTexture, 0, sceneDepthRTV.ReleaseAndGetAddressOf());

	// Create the Shader Resource Views
	device->CreateShaderResourceView(ppTexture, 0, ppSRV.ReleaseAndGetAddressOf());
	device->CreateShaderResourceView(sceneNormalsTexture, 0, sceneNormalsSRV.ReleaseAndGetAddressOf());
	device->CreateShaderResourceView(sceneDepthsTexture, 0, sceneDepthSRV.ReleaseAndGetAddressOf());

	// Release the extra texture references
	ppTexture->Release();
	sceneNormalsTexture->Release();
	sceneDepthsTexture->Release();
}

void Game::LightControl(float dt)
{
	switch (controlMode) 
	{
	case CONTROL_MODE_MOVE_DIRLIGHT:
		// direction
		if ((GetAsyncKeyState('W') & 0x8000) && (dirLightDirection.z <= 1)) { dirLightDirection.z += dt * 0.1f; }
		if ((GetAsyncKeyState('S') & 0x8000) && (dirLightDirection.z >= -1)) { dirLightDirection.z -= dt * 0.1f; }
		if ((GetAsyncKeyState('A') & 0x8000) && (dirLightDirection.x >= -1)) { dirLightDirection.x -= dt * 0.1f; }
		if ((GetAsyncKeyState('D') & 0x8000) && (dirLightDirection.x <= 1)) { dirLightDirection.x += dt * 0.1f; }
		if ((GetAsyncKeyState('Q') & 0x8000) && (dirLightDirection.y <= 1)) { dirLightDirection.y += dt * 0.1f; }
		if ((GetAsyncKeyState('E') & 0x8000) && (dirLightDirection.y >= -1)) { dirLightDirection.y -= dt * 0.1f; }

		// since directional light, update shadow map
		UpdateShadowMapView();

        break;
    case CONTROL_MODE_MOVE_POINTLIGHT:
		// position
        if ((GetAsyncKeyState('W') & 0x8000) && (pointLightPosition.z <= 20)) { pointLightPosition.z += dt * 1.0f; }
        if ((GetAsyncKeyState('S') & 0x8000) && (pointLightPosition.z >= -20)) { pointLightPosition.z -= dt * 1.0f; }
        if ((GetAsyncKeyState('A') & 0x8000) && (pointLightPosition.x >= -20)) { pointLightPosition.x -= dt * 1.0f; }
        if ((GetAsyncKeyState('D') & 0x8000) && (pointLightPosition.x <= 20)) { pointLightPosition.x += dt * 1.0f; }
        if ((GetAsyncKeyState('Q') & 0x8000) && (pointLightPosition.y <= 20)) { pointLightPosition.y += dt * 1.0f; }
		if ((GetAsyncKeyState('E') & 0x8000) && (pointLightPosition.y >= -20)) { pointLightPosition.y -= dt * 1.0f; }

		// range
		if ((GetAsyncKeyState(VK_UP) & 0x8000) && (pointLightRange < 100)) { pointLightRange += dt * 5.0f; }
		if ((GetAsyncKeyState(VK_DOWN) & 0x8000) && (pointLightRange > 0)) { pointLightRange -= dt * 5.0f; }

        break;
    case CONTROL_MODE_MOVE_SPOTLIGHT:
		// position
        if ((GetAsyncKeyState('W') & 0x8000) && (spotLightPosition.z <= 20)) { spotLightPosition.z += dt * 1.0f; }
        if ((GetAsyncKeyState('S') & 0x8000) && (spotLightPosition.z >= -20)) { spotLightPosition.z -= dt * 1.0f; }
        if ((GetAsyncKeyState('A') & 0x8000) && (spotLightPosition.x >= -20)) { spotLightPosition.x -= dt * 1.0f; }
        if ((GetAsyncKeyState('D') & 0x8000) && (spotLightPosition.x <= 20)) { spotLightPosition.x += dt * 1.0f; }
        if ((GetAsyncKeyState('Q') & 0x8000) && (spotLightPosition.y <= 20)) { spotLightPosition.y += dt * 1.0f; }
        if ((GetAsyncKeyState('E') & 0x8000) && (spotLightPosition.y >= -20)) { spotLightPosition.y -= dt * 1.0f; }

		// direction
		if ((GetAsyncKeyState('I') & 0x8000) && (spotLightDirection.z <= 1)) {	spotLightDirection.z += dt * 0.1f; }
		if ((GetAsyncKeyState('K') & 0x8000) && (spotLightDirection.z >= -1)) { spotLightDirection.z -= dt * 0.1f; }
		if ((GetAsyncKeyState('J') & 0x8000) && (spotLightDirection.x >= -1)) { spotLightDirection.x -= dt * 0.1f; }
		if ((GetAsyncKeyState('L') & 0x8000) && (spotLightDirection.x <= 1)) {	spotLightDirection.x += dt * 0.1f; }
		if ((GetAsyncKeyState('U') & 0x8000) && (spotLightDirection.y <= 1)) {	spotLightDirection.y += dt * 0.1f; }
		if ((GetAsyncKeyState('O') & 0x8000) && (spotLightDirection.y >= -1)) { spotLightDirection.y -= dt * 0.1f; }

		// range
		if ((GetAsyncKeyState(VK_UP) & 0x8000) && (spotLightRange < 100)) { spotLightRange += dt * 5.0f; }
		if ((GetAsyncKeyState(VK_DOWN) & 0x8000) && (spotLightRange > 0)) { spotLightRange -= dt * 5.0f; }

		// falloff
		if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) && (spotLightFalloff < 100)) { spotLightFalloff += dt * 5.0f; }
		if ((GetAsyncKeyState(VK_LEFT) & 0x8000) && (spotLightFalloff > 1)) { spotLightFalloff -= dt * 5.0f; }

		break;
	default:
		break;
	}

	UpdateLights();
}

void Game::ToggleLights(int light)
{
	// check if the light is already on
	if (1 == lights[(size_t)light - 1].enabled)
		return;

	// turn off all lights, turn on correct one
	for (size_t i = 0; i < lights.size(); i++)
	{
		lights[i].enabled = 0;
	}

	lights[(size_t)light - 1].enabled = 1;
}

void Game::UpdateLights()
{
	if (0 == controlMode || controlMode > lights.size())
		return;

	Light lightToUpdate = lights[(size_t)controlMode - 1];

	switch (controlMode)
	{
	case CONTROL_MODE_MOVE_DIRLIGHT:
		lightToUpdate.direction = dirLightDirection;
		break;
	case CONTROL_MODE_MOVE_POINTLIGHT:
		lightToUpdate.position = pointLightPosition;
		lightToUpdate.radius = pointLightRange;
		break;
	case CONTROL_MODE_MOVE_SPOTLIGHT:
		lightToUpdate.position = spotLightPosition;
		lightToUpdate.direction = spotLightDirection;
		lightToUpdate.spotPower = spotLightFalloff;
		lightToUpdate.radius = spotLightRange;
		break;
	default:
		break;
	}

	lights[(size_t)controlMode - 1] = lightToUpdate;
}

void Game::PreRender()
{
	// Background color (Cornflower Blue in this case) for clearing
	const float color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Clear the render target and depth buffer (erases what's on the screen)
	//  - Do this ONCE PER FRAME
	//  - At the beginning of Draw (before drawing *anything*)
	context->ClearRenderTargetView(backBufferRTV.Get(), color);
	context->ClearDepthStencilView(
		depthStencilView.Get(),
		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
		1.0f,
		0);

	// Ensure we are clearing all render targets
	context->ClearRenderTargetView(ppRTV.Get(), color);
	context->ClearRenderTargetView(sceneNormalsRTV.Get(), color);
	context->ClearRenderTargetView(sceneDepthRTV.Get(), color);

	// Set all 3 render targets, making all three active at once
	// - Properly utilizing these all at once requires a special setup
	//   in your pixel shader, so that the shader returns multiple colors
	ID3D11RenderTargetView* rtvs[3] =
	{
		ppRTV.Get(),
		sceneNormalsRTV.Get(),
		sceneDepthRTV.Get()
	};
	context->OMSetRenderTargets(3, rtvs, depthStencilView.Get());
	context->ClearRenderTargetView(ppRTV.Get(), color);
}

void Game::PostRender()
{
	// Necessary for potentially turning off buffers
	UINT stride = sizeof(Vertex);
	UINT offset = 0;
	ID3D11Buffer* nothing = 0;

	// do depth normal outlines
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), 0);
	// Set up post process shaders
	ppVS->SetShader();

	ppPS->SetShaderResourceView("pixels", ppSRV.Get());
	ppPS->SetShaderResourceView("normals", sceneNormalsSRV.Get());
	ppPS->SetShaderResourceView("depth", sceneDepthSRV.Get());
	ppPS->SetSamplerState("samplerOptions", clampSampler.Get());
	ppPS->SetShader();
	
	ppPS->SetFloat("pixelWidth", 1.0f / width);
	ppPS->SetFloat("pixelHeight", 1.0f / height);
	ppPS->SetFloat("depthAdjust", 5.0f);
	ppPS->SetFloat("normalAdjust", 5.0f);
	ppPS->CopyAllBufferData();

	// Turn OFF my vertex and index buffers
	context->IASetIndexBuffer(0, DXGI_FORMAT_R32_UINT, 0);
	context->IASetVertexBuffers(0, 1, &nothing, &stride, &offset);

	// Draw exactly 3 vertices, which the special post-process vertex shader will
	// "figure out" on the fly (resulting in our "full screen triangle")
	context->Draw(3, 0);

	// Unbind shader resource views at the end of the frame,
	// since we'll be rendering into one of those textures
	// at the start of the next
	ID3D11ShaderResourceView* nullSRVs[16] = {};
	context->PSSetShaderResources(0, 16, nullSRVs);
}

void Game::DrawUI()
{
	spriteBatch->Begin();

	// Title
	spriteFont->DrawString(spriteBatch.get(), "Toon Shader Sandbox", XMFLOAT2(10, 10), Colors::LawnGreen);

	// Description
	spriteFont->DrawString(spriteBatch.get(), "This project presents Toon/Cel Shading through\nvarious lighting manipulation alongside\nthe use of real-time shadows.", XMFLOAT2(10, 30), Colors::LawnGreen);

	// Controls
	spriteFont->DrawString(spriteBatch.get(), "== Controls ==", XMFLOAT2(10, 100), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "TAB: Change control mode\n", XMFLOAT2(10, 120), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "V: Toggle Real-Time Shadows\n", XMFLOAT2(10, 140), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "1: Directional Light", XMFLOAT2(10, 160), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "2: Point Light", XMFLOAT2(10, 180), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "3: Spot Light", XMFLOAT2(10, 200), Colors::LawnGreen);

	// Info on current outline mode
	spriteFont->DrawString(spriteBatch.get(), "== Control Mode ==", XMFLOAT2(10, 260), Colors::LawnGreen);
	spriteFont->DrawString(spriteBatch.get(), "Current Mode:", XMFLOAT2(10, 280), Colors::LawnGreen);

	switch (controlMode) 
	{
	case CONTROL_MODE_MOVE_CAMERA:
		spriteFont->DrawString(spriteBatch.get(), "Camera Mode", XMFLOAT2(120, 280), Colors::LightSeaGreen);
		spriteFont->DrawString(spriteBatch.get(), "Use WASD to move around\nClick and Drag to look", XMFLOAT2(10, 300), Colors::LawnGreen);
		break;
	case CONTROL_MODE_MOVE_DIRLIGHT:
		spriteFont->DrawString(spriteBatch.get(), "Direction Light Mode", XMFLOAT2(120, 280), Colors::LightSeaGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use WASDQE to change the lighting direction", XMFLOAT2(10, 300), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Click and Drag to look", XMFLOAT2(10, 320), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "Direction:", XMFLOAT2(10, 360), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "X:", XMFLOAT2(10, 380), Colors::HotPink);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(dirLightDirection.x).c_str(), XMFLOAT2(30, 380), Colors::Red);
		spriteFont->DrawString(spriteBatch.get(), "Y:", XMFLOAT2(10, 400), Colors::LightGreen);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(dirLightDirection.y).c_str(), XMFLOAT2(30, 400), Colors::Green);
		spriteFont->DrawString(spriteBatch.get(), "Z:", XMFLOAT2(10, 420), Colors::LightBlue);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(dirLightDirection.z).c_str(), XMFLOAT2(30, 420), Colors::Blue);
		break;
	case CONTROL_MODE_MOVE_POINTLIGHT:
		spriteFont->DrawString(spriteBatch.get(), "Point Light Mode", XMFLOAT2(120, 280), Colors::LightSeaGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use WASDQE to change the point light position", XMFLOAT2(10, 300), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use Up/Down Arrow to adjust range", XMFLOAT2(10, 320), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Click and Drag to look", XMFLOAT2(10, 340), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "Position:", XMFLOAT2(10, 380), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "X:", XMFLOAT2(10, 400), Colors::HotPink);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(pointLightPosition.x).c_str(), XMFLOAT2(30, 400), Colors::Red);
		spriteFont->DrawString(spriteBatch.get(), "Y:", XMFLOAT2(10, 420), Colors::LightGreen);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(pointLightPosition.y).c_str(), XMFLOAT2(30, 420), Colors::Green);
		spriteFont->DrawString(spriteBatch.get(), "Z:", XMFLOAT2(10, 440), Colors::LightBlue);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(pointLightPosition.z).c_str(), XMFLOAT2(30, 440), Colors::Blue);
		spriteFont->DrawString(spriteBatch.get(), "Range:", XMFLOAT2(10, 480), Colors::Cyan);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(pointLightRange).c_str(), XMFLOAT2(65, 480), Colors::LightCyan);
		break;
	case CONTROL_MODE_MOVE_SPOTLIGHT:
		spriteFont->DrawString(spriteBatch.get(), "Spot Light Mode", XMFLOAT2(120, 280), Colors::LightSeaGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use WASDQE to change the spot light position", XMFLOAT2(10, 300), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use IJKLUO to change the spot light direction", XMFLOAT2(10, 320), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use Up/Down Arrow to adjust range", XMFLOAT2(10, 340), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Use Left/Right Arrow to adjust falloff", XMFLOAT2(10, 360), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "- Click and Drag to look", XMFLOAT2(10, 380), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "Position:", XMFLOAT2(10, 420), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "X:", XMFLOAT2(10, 440), Colors::HotPink);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightPosition.x).c_str(), XMFLOAT2(30, 440), Colors::Red);
		spriteFont->DrawString(spriteBatch.get(), "Y:", XMFLOAT2(10, 460), Colors::LightGreen);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightPosition.y).c_str(), XMFLOAT2(30, 460), Colors::Green);
		spriteFont->DrawString(spriteBatch.get(), "Z:", XMFLOAT2(10, 480), Colors::LightBlue);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightPosition.z).c_str(), XMFLOAT2(30, 480), Colors::Blue);
		spriteFont->DrawString(spriteBatch.get(), "Direction:", XMFLOAT2(10, 500), Colors::LawnGreen);
		spriteFont->DrawString(spriteBatch.get(), "X:", XMFLOAT2(10, 520), Colors::HotPink);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightDirection.x).c_str(), XMFLOAT2(30, 520), Colors::Red);
		spriteFont->DrawString(spriteBatch.get(), "Y:", XMFLOAT2(10, 540), Colors::LightGreen);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightDirection.y).c_str(), XMFLOAT2(30, 540), Colors::Green);
		spriteFont->DrawString(spriteBatch.get(), "Z:", XMFLOAT2(10, 560), Colors::LightBlue);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightDirection.z).c_str(), XMFLOAT2(30, 560), Colors::Blue);
		spriteFont->DrawString(spriteBatch.get(), "Range:", XMFLOAT2(10, 600), Colors::Cyan);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightRange).c_str(), XMFLOAT2(65, 600), Colors::LightCyan);
		spriteFont->DrawString(spriteBatch.get(), "Falloff:", XMFLOAT2(10, 620), Colors::Cyan);
		spriteFont->DrawString(spriteBatch.get(), std::to_string(spotLightFalloff).c_str(), XMFLOAT2(65, 620), Colors::LightCyan);
		break;
	default:
		break;
	}

	spriteBatch->End();

	// Reset render states altered by sprite batch!
	context->RSSetState(0);
	context->OMSetDepthStencilState(0, 0);
	context->OMSetBlendState(0, 0, 0xFFFFFFFF);
}

void Game::RenderShadowMap()
{
	// Set the current render target and depth buffer
	// for shadow map creations
	// (Changing where the rendering goes!)
	context->OMSetRenderTargets(0, 0, shadowDSV.Get()); // Only need the depth buffer (shadow map)
	context->ClearDepthStencilView(shadowDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	// Change any shadow-mapping-specific render states
	context->RSSetState(shadowRasterizer.Get());

	// Create a viewport to match the new target size
	D3D11_VIEWPORT vp = {};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)shadowMapSize;
	vp.Height = (float)shadowMapSize;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);

	// Set up vertex and pixel shaders
	shadowVS->SetShader();
	shadowVS->SetMatrix4x4("view", shadowViewMatrix);
	shadowVS->SetMatrix4x4("projection", shadowProjectionMatrix);
	context->PSSetShader(0, 0, 0); // Turns OFF the pixel shader!

	// Loop and render all entities
	for (auto& e : entities)
	{
		// Grab this entity's world matrix and
		// send to the VS
		shadowVS->SetMatrix4x4("world", e->GetTransform()->GetWorldMatrix());
		shadowVS->CopyAllBufferData();

		// Only draw the current entity
		// Set the Vertex and Index Buffer
		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, e->GetMesh()->GetVertexBuffer().GetAddressOf(), &stride, &offset);
		context->IASetIndexBuffer(e->GetMesh()->GetIndexBuffer().Get(), DXGI_FORMAT_R32_UINT, 0);

		// tell D3D to render using the currently bound resources
		// Finally do the actual drawing
			//  - Do this ONCE PER OBJECT you intend to draw
			//  - This will use all of the currently set DirectX "stuff" (shaders, buffers, etc)
			//  - DrawIndexed() uses the currently set INDEX BUFFER to look up corresponding
			//     vertices in the currently set VERTEX BUFFER
		context->DrawIndexed(
			e->GetMesh()->GetIndexCount(),     // The number of indices to use (we could draw a subset if we wanted)
			0,     // Offset to the first index we want to use
			0);    // Offset to add to each index when looking up vertices
	}

	// Reset anything I've changed
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
	vp.Width = (float)this->width;
	vp.Height = (float)this->height;
	context->RSSetViewports(1, &vp);
	context->RSSetState(0);
}

void Game::UpdateShadowMapView()
{
	// Create the view and projection for the shadow map
	XMMATRIX shView = XMMatrixLookToLH(
		XMVectorSet(-15*dirLightDirection.x, -15* dirLightDirection.y, -15*dirLightDirection.z, 0), // "Backing up" along negative light dir
		XMVectorSet(dirLightDirection.x, dirLightDirection.y, dirLightDirection.z, 0), // This should always match the light's dir
		XMVectorSet(0, 1, 0, 0));
	XMStoreFloat4x4(&shadowViewMatrix, shView);
}

// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// update the camera projection matrix
	if (mainCamera)
	{
		mainCamera->UpdateProjectionMatrix((float)this->width / this->height);
	}

	// resize post process resources
	ResizePostProcessResources();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// update entities transformation

	// floor
	entities[0]->GetTransform()->SetPosition(0.0f, -5.0f, 0.0f);
	entities[0]->GetTransform()->SetScale(15.0f, 1.0f, 15.f);

	// front wall
	entities[1]->GetTransform()->SetPosition(0.0f, -0.5f, -8.0f);
	entities[1]->GetTransform()->SetScale(15.0f, 10.0f, 1.0f);

	// back wall
	entities[2]->GetTransform()->SetPosition(0.0f, -0.5f, 8.0f);
	entities[2]->GetTransform()->SetScale(15.0f, 10.0f, 1.0f);

	// left wall
	entities[3]->GetTransform()->SetPosition(-8.0f, -0.5f, 0.0f);
	entities[3]->GetTransform()->SetScale(1.0f, 10.0f, 17.0f);

	// right wall
	entities[4]->GetTransform()->SetPosition(8.0f, -0.5f, 0.0f);
	entities[4]->GetTransform()->SetScale(1.0f, 10.0f, 17.0f);

	// tv table
	entities[5]->GetTransform()->SetScale(0.02f, 0.02f, 0.02f);
	entities[5]->GetTransform()->SetPosition(0.0f, -4.5f, -4.5f);

	// sofa
	entities[6]->GetTransform()->SetScale(0.03f, 0.03f, 0.03f);
	entities[6]->GetTransform()->SetPosition(0.0f, -4.5f, 4.5f);

	// tv
	entities[7]->GetTransform()->SetScale(5.0f, 5.0f, 5.0f);
	entities[7]->GetTransform()->SetPitchYawRoll(0.0f, XM_PI, 0.0f);
	entities[7]->GetTransform()->SetPosition(0.0f, -2.85f, -4.5f);

	// coffee table
	entities[8]->GetTransform()->SetScale(1.5f, 1.5f, 1.5f);
	entities[8]->GetTransform()->SetPosition(0.0f, -4.6f, 0.0f);

	// newton's cradle
	entities[9]->GetTransform()->SetScale(0.05f, 0.05f, 0.05f);
	entities[9]->GetTransform()->SetPosition(0.0f, -2.95f, 0.0f);

	// claymore sword
	entities[10]->GetTransform()->SetScale(0.05f, 0.05f, 0.05f);
	entities[10]->GetTransform()->SetPosition(1.0f, -3.76f, 0.0f);
	entities[10]->GetTransform()->SetPitchYawRoll(-0.01f, XM_PI/4, 0.0f);

	// update the camera
	if (mainCamera)
	{
		mainCamera->Update(deltaTime, this->hWnd, controlMode);
	}

	if (controlMode > 0)
	{
		LightControl(deltaTime);
	}

	if (GetAsyncKeyState('1') & 0x8000) { ToggleLights(1); }
	if (GetAsyncKeyState('2') & 0x8000) { ToggleLights(2); }
	if (GetAsyncKeyState('3') & 0x8000) { ToggleLights(3); }

	bool currentTab = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
	if(currentTab && !prevTab)
	{
		controlMode++;
		controlMode = controlMode % (CONTROL_MODE_MOVE_SPOTLIGHT + 1);
	}

	bool currentV = (GetAsyncKeyState('V') & 0x8000) != 0;
	if (currentV && !prevV) { enableShadows = !enableShadows; }

	prevTab = currentTab;
	prevV = currentV;

	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Render shadow map
	if(enableShadows)
		RenderShadowMap();

	// clear render target and depth buffer
	PreRender();

	// loop through entity list and draw them
	for (size_t i = 0; i < entities.size(); i++) 
	{
		// Set data for each entity since there are multiple shader files
		SimpleVertexShader * entityVS = entities[i]->GetMaterial()->GetVertexShader();
		SimplePixelShader * entityPS = entities[i]->GetMaterial()->GetPixelShader();
		entityPS->SetData("lights", (void*)(&lights[0]), sizeof(Light) * MAX_LIGHTS);
		entityPS->SetInt("lightCount", (int)lights.size());
		entityPS->SetInt("renderShadows", (int)enableShadows);
		entityPS->SetFloat3("cameraPos", mainCamera->GetTransform()->GetPosition());
		entityPS->SetFloat("specularIntensity", entities[i]->GetMaterial()->GetSpecularIntensity());
		entityPS->CopyAllBufferData();
		entityPS->SetSamplerState("ClampSampler", clampSampler.Get());
		entityPS->SetSamplerState("shadowSampler", shadowSampler.Get());
		entityPS->SetShaderResourceView("RampMap", toonRamp_SRV.Get());
		entityPS->SetShaderResourceView("specularRampMap", specularToonRamp_SRV.Get()); 
		entityPS->SetShaderResourceView("shadowMap", shadowSRV.Get());
		entityVS->SetMatrix4x4("shadowView", shadowViewMatrix);
		entityVS->SetMatrix4x4("shadowProjection", shadowProjectionMatrix);
		entities[i]->Draw(device, context, mainCamera);
	}

	// draw the SkyBox
	skyBox->Draw(context, mainCamera);

	// post processing
	PostRender();

	// Draw UI
	DrawUI();

	// Present the back buffer to the user
	//  - Puts the final frame we're drawing into the window so the user can see it
	//  - Do this exactly ONCE PER FRAME (always at the very end of the frame)
	swapChain->Present(0, 0);

	// Due to the usage of a more sophisticated swap chain,
	// the render target must be re-bound after every call to Present()
	context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthStencilView.Get());
}