#ifndef __GGP_LIGHTING__
#define __GGP_LIGHTING__

#include "PBRIncludes.hlsli"

#define TYPE_DIRECTIONAL    0
#define TYPE_POINT          1
#define TYPE_SPOT           2

struct Light
{
    int Type;
    float3 Direction;
	//-----------------
    float Radius;
    float3 Position;
	//-----------------
    float Intensity;
    float3 DiffuseColor;
	//-----------------
    float SpotPower;
    float3 AmbientColor;
	//-----------------
	int Enabled;
	float3 Padding;
};

// utility functions
float getAttenuation(float3 lightPos, float3 pixelPos, float radius)
{
    float dist = distance(lightPos, pixelPos);
    float att = 1.0f - (dist * dist / (radius * radius));

    // clamp and return
    return saturate(att);
}

// lighting functions
float diffuseLighting(float3 normal, float3 lightDir)
{
    return saturate(dot(normal, lightDir));
}

float specularPhong(float3 normal, float3 lightDir, float3 cameraPos, float3 worldPos, float diffuse, float specExponent)
{
    // get V
    float3 viewVector = normalize(cameraPos - worldPos);
    // get R
    float3 reflection = reflect(lightDir, normal);

    // calculate specular
    float spec = pow(saturate(dot(reflection, viewVector)), specExponent);
    spec *= any(diffuse);

    return spec;
}

// toon lighting functions
float ApplyToonShadingRamp(float originalNdotL, Texture2D rampTexture, SamplerState clampSampler)
{
	// Use the original N dot L (a value between 0 and 1) as a sampling
	// location in the ramp texture.  The red channel sampled from that
	// texture will be returned as the new N dot L value.
	return rampTexture.Sample(clampSampler, float2(originalNdotL, 0)).r;
}

// --------------------------------------------------------------------
// ---------------- BASIC LIGHTING CALCULATIONS -----------------------
// --------------------------------------------------------------------
float3 dLightColor(float3 normal, float3 worldPos, Light light, float3 cameraPos, float3 surfaceColor, float specularIntensity)
{
	// get normalized direction to light
	float3 lightDirection = normalize(-light.Direction);

	// get Diffuse and Specular
	float diffuse = diffuseLighting(normal, lightDirection);
	float specular = specularPhong(normal, lightDirection, cameraPos, worldPos, diffuse, 64);
	specular = specular * specularIntensity;

	float3 diffuseLightColor = diffuse * light.DiffuseColor;
	return (diffuseLightColor * surfaceColor + specular) * light.Intensity * light.AmbientColor;
}

float3 pLightColor(float3 normal, float3 worldPos, Light light, float3 cameraPos, float3 surfaceColor, float specularIntensity)
{
	// first get the vector from position of light to vertex
	float3 lightDirection = normalize(light.Position - worldPos);

	// diffuse and specular calculations
	float diffuse = diffuseLighting(normal, lightDirection);
	float specular = specularPhong(normal, lightDirection, cameraPos, worldPos, diffuse, 64);
	specular = specular * specularIntensity;

	// attenuate
	float att = getAttenuation(light.Position, worldPos, light.Radius);

	// simplify a bit before returning
	float3 diffuseLightColor = diffuse * light.DiffuseColor;
	return (diffuseLightColor * surfaceColor + specular) * att * light.Intensity * light.AmbientColor;
}

float3 sLightColor(float3 normal, float3 worldPos, Light light, float3 cameraPos, float3 surfaceColor, float specularIntensity)
{
	// first get the vector from position of light to vertex
	float3 lightDirection = normalize(light.Position - worldPos);
	
	// diffuse and specular
	float diffuse = diffuseLighting(normal, lightDirection);
	float specular = specularPhong(normal, lightDirection, cameraPos, worldPos, diffuse, 64);
	specular = specular * specularIntensity;

	// get attenuation
	float att = getAttenuation(light.Position, worldPos, light.Radius);

	// do spot light calcultions, calculate falloff
	float angleFromCenter = saturate(dot(-lightDirection, normalize(light.Direction)));
	float spotAmount = pow(angleFromCenter, light.SpotPower);

	float3 diffuseLightColor = diffuse * light.DiffuseColor;
	return (diffuseLightColor * surfaceColor + specular) * att * spotAmount * light.Intensity * light.AmbientColor;
}

// --------------------------------------------------------------------
// ---------------- PBR LIGHTING CALCULATIONS -------------------------
// --------------------------------------------------------------------
float3 dLightColorPBR(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(-light.Direction);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate diffuse (nDotL) and specular (Cook-Torrance)
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);

	float3 total = (balancedDiff * surfaceColor + specular) * light.Intensity * light.AmbientColor;
	return total;
}

float3 pLightColorPBR(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(light.Position - worldPos);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate attenuation, diffuse (nDotL) and specular (Cook-Torrance)
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);

	// get attenuation
	float att = getAttenuation(light.Position, worldPos, light.Radius);

	float3 total = (balancedDiff * surfaceColor + specular) * att * light.Intensity * light.AmbientColor;
	return total;
}

float3 sLightColorPBR(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(light.Position - worldPos);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate attenuation, diffuse (nDotL) and specular (Cook-Torrance)
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);

	// get attenuation
	float att = getAttenuation(light.Position, worldPos, light.Radius);

	// do spot light calcultions
	float angleFromCenter = saturate(dot(-lightDirection, normalize(light.Direction)));
	float spotAmount = pow(angleFromCenter, light.SpotPower);

	float3 total = (balancedDiff * surfaceColor + specular) * att * spotAmount * light.Intensity * light.AmbientColor;
	return total;
}

// --------------------------------------------------------------------
// ---------------- TOON LIGHTING CALCULATIONS ------------------------
// --------------------------------------------------------------------
float3 dLightColorPBRToon(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor, Texture2D rampTexture, Texture2D specularRampTexture, SamplerState clampSampler)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(-light.Direction);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate diffuse (nDotL) and specular (Cook-Torrance)
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// Test Toon Shading
	float toonDiffuse = ApplyToonShadingRamp(diffuse, rampTexture, clampSampler);
	float luminance = dot(specular, float3(0.2125f, 0.7154f, 0.0721f));
	float toonLuminance = ApplyToonShadingRamp(luminance, specularRampTexture, clampSampler);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);

	float3 toonSpec = specColor * toonLuminance;
	float3 balancedToonDiff = DiffuseEnergyConserve(toonDiffuse, toonSpec, metalness);

	float3 toonDiffuseLightColor = toonDiffuse * light.DiffuseColor;
	float3 total = (toonDiffuseLightColor * surfaceColor + toonSpec) * light.Intensity + light.AmbientColor;
	return total;
}

float3 pLightColorPBRToon(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor, Texture2D rampTexture, Texture2D specularRampTexture, SamplerState clampSampler)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(light.Position - worldPos);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate attenuation, diffuse (nDotL) and specular (Cook-Torrance)
	float att = getAttenuation(light.Position, worldPos, light.Radius);
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// Test Toon Shading
	float toonDiffuse = ApplyToonShadingRamp(diffuse, rampTexture, clampSampler);
	float luminance = dot(specular, float3(0.2125f, 0.7154f, 0.0721f));
	float toonLuminance = ApplyToonShadingRamp(luminance, specularRampTexture, clampSampler);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);
	float3 toonSpec = specColor * toonLuminance;
	float3 balancedToonDiff = DiffuseEnergyConserve(toonDiffuse, toonSpec, metalness);

	float3 toonDiffuseLightColor = toonDiffuse * light.DiffuseColor;
	float3 total = (toonDiffuseLightColor * surfaceColor + toonSpec) * att * light.Intensity + light.AmbientColor;
	return total;
}

float3 sLightColorPBRToon(float3 normal, float3 worldPos, Light light, float3 cameraPos, float metalness, float roughness, float3 surfaceColor, float3 specColor, Texture2D rampTexture, Texture2D specularRampTexture, SamplerState clampSampler)
{
	// get and normalize light and camera direction
	float3 lightDirection = normalize(light.Position - worldPos);
	float3 cameraDirection = normalize(cameraPos - worldPos);

	// calculate attenuation, diffuse (nDotL) and specular (Cook-Torrance)
	float att = getAttenuation(light.Position, worldPos, light.Radius);
	float diffuse = diffuseLighting(normal, lightDirection);
	float3 specular = MicrofacetBRDF(normal, lightDirection, cameraDirection, roughness, metalness, specColor);

	// Toon Shading
	float toonDiffuse = ApplyToonShadingRamp(diffuse, rampTexture, clampSampler);
	float luminance = dot(specular, float3(0.2125f, 0.7154f, 0.0721f));
	float toonLuminance = ApplyToonShadingRamp(luminance, specularRampTexture, clampSampler);

	// calculate energy conservation
	float3 balancedDiff = DiffuseEnergyConserve(diffuse, specular, metalness);
	float3 toonSpec = specColor * toonLuminance;
	float3 balancedToonDiff = DiffuseEnergyConserve(toonDiffuse, toonSpec, metalness);

	// do spot light calcultions
	float angleFromCenter = saturate(dot(-lightDirection, normalize(light.Direction)));
	float spotAmount = pow(angleFromCenter, light.SpotPower);

	float3 toonDiffuseLightColor = toonDiffuse * light.DiffuseColor;
	float3 total = (toonDiffuseLightColor * surfaceColor + toonSpec) * att * spotAmount * light.Intensity + light.AmbientColor;
	return total;
}
#endif