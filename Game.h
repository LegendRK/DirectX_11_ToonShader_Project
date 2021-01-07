#pragma once

#include "DXCore.h"
#include "Mesh.h"
#include "BufferStructs.h"
#include "Entity.h"
#include "Camera.h"
#include "Material.h"
#include "SimpleShader.h"
#include "Lights.h"
#include "SkyBox.h"
#include "WICTextureLoader.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include <vector>

#define CONTROL_MODE_MOVE_CAMERA		0
#define CONTROL_MODE_MOVE_DIRLIGHT		1
#define CONTROL_MODE_MOVE_POINTLIGHT	2
#define CONTROL_MODE_MOVE_SPOTLIGHT		3

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, etc.
	void LoadShaders();
	void LoadBasicTexture(const wchar_t* file, ID3D11ShaderResourceView** textureSRV);
	void LoadPBRTexture(const wchar_t* albedoPath, ID3D11ShaderResourceView** albedoSRV, const wchar_t* normalPath, ID3D11ShaderResourceView** normalSRV, const wchar_t* metalPath, ID3D11ShaderResourceView** metalSRV, const wchar_t* roughnessPath, ID3D11ShaderResourceView** roughnessSRV);
	void LoadTextures();
	void CreateBasicGeometry();
	void GenerateLights();
	void InitializeShadowMap();

	void ResizePostProcessResources();

	void LightControl(float dt);
	void ToggleLights(int light);
	void UpdateLights();

	void PreRender();
	void PostRender();

	void DrawUI();

	void RenderShadowMap();
	void UpdateShadowMapView();
	
	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//    Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr
	
	// Shaders and shader-related constructs
	SimpleVertexShader* vertexShader;
	SimplePixelShader* pixelShader;

	SimpleVertexShader* skyVertexShader;
	SimplePixelShader* skyPixelShader;

	SimpleVertexShader* ppVS;
	SimplePixelShader* ppPS;

	SimpleVertexShader* shadowVS;

	// List of entites
	std::vector<Entity*> entities;
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;

	Camera* mainCamera;

	// keep track of modes
	int controlMode = 0;
	bool prevTab;
	bool prevV;

	// controllable vars for lights
	DirectX::XMFLOAT3 dirLightDirection;

	DirectX::XMFLOAT3 pointLightPosition;
	float pointLightRange = 0.0f;

	DirectX::XMFLOAT3 spotLightDirection;
	DirectX::XMFLOAT3 spotLightPosition;
	float spotLightRange = 0.0f;
	float spotLightFalloff = 0.0f;

	bool enableShadows;

	// Sprite batch and sprite font for 2D rendering
	std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
	std::unique_ptr<DirectX::SpriteFont> spriteFont;
	std::unique_ptr<DirectX::SpriteFont> spriteFontLarge;

	// skybox
	SkyBox* skyBox;

	// lights
	std::vector<Light> lights;

	// post processing vars
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> ppRTV;		// Allows us to render to a texture
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ppSRV;		// Allows us to sample from the same texture

	// Depth/normal technique
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneDepthRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneDepthSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> sceneNormalsRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sceneNormalsSRV;

	// shadow map resources
	int shadowMapSize;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> shadowDSV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRV;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSampler;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRasterizer;
	DirectX::XMFLOAT4X4 shadowViewMatrix;
	DirectX::XMFLOAT4X4 shadowProjectionMatrix;

	// samplers
	Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> clampSampler;

	// textures
	// A - Albedo
	// N - Normal
	// M - Metallness
	// R - Roughness

	// carpet
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> carpetA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> carpetN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> carpetM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> carpetR_SRV;

	// walls
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wallR_SRV;

	// table
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tableA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tableN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tableM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tableR_SRV;

	// sofa
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sofaA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sofaN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sofaM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> sofaR_SRV;

	// tv
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tvA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tvN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tvM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> tvR_SRV;

	// coffee table
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coffeeTableA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coffeeTableN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coffeeTableM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> coffeeTableR_SRV;

	// newton's cradle
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cradleA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cradleN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cradleM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> cradleR_SRV;

	// sword
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> swordA_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> swordN_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> swordM_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> swordR_SRV;

	// toon shader ramps
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> toonRamp_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> toonRamp1_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> toonRamp2_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> toonRamp3_SRV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> specularToonRamp_SRV;
};

