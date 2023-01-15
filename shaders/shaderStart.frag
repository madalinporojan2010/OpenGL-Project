#version 410 core


in vec4 mainFragPosLightSpace;

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;


out vec4 fColor;


//lighting
uniform	vec3 mainLightDir;
uniform	vec3 secondaryLightDir;

uniform	vec3 mainLightColor;
uniform	vec3 secondaryLightColor;

//texture
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

struct LightStruct {
	float ambientStrength;
	float specularStrength;
	float shininess;
	vec3 lightColor;
	vec3 lightDir;
	vec4 fragPosLightSpace;
} mainLight, secondaryLight;

float computeShadow(LightStruct light) {
	vec3 normalizedCoords = light.fragPosLightSpace.xyz / light.fragPosLightSpace.w;
	normalizedCoords = normalizedCoords * 0.5f + 0.5f;
	if (normalizedCoords.z > 1.0f) {
		return 0.0f;
	}

	float closestDepth = texture(shadowMap, normalizedCoords.xy).r;
	float currentDepth = normalizedCoords.z;
	float bias = max(0.0f * (1.0f - dot(fNormal, light.lightDir)), 0.005f);
	float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
	return shadow;
}

float computeFog() {
	float fogDensity = 0.02f;
	float fragmentDistance = length(fPosEye);
	float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2));
	
	return clamp(fogFactor, 0.0f, 1.0f);
}


vec3 computeLightComponents(LightStruct light, bool mainLight, bool punctiform)
{		
	float shadow = 0.0f;
	if(mainLight)
		shadow = computeShadow(light);
	vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
	
	//transform normal
	vec3 normalEye = normalize(fNormal);	
	
	//compute light direction
	vec3 lightDirN = normalize(light.lightDir - fPosEye.xyz);
	
	//compute view direction 
	vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);
		
	//compute ambient light
	vec3 ambient;
	vec3 diffuse;
	//compute specular light
	vec3 reflection = reflect(-lightDirN, normalEye);
	float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), light.shininess);
	vec3 specular;
	if (punctiform) {	
		float constant = 1.0f;
		float linear = 0.0045f;
		float quadratic = 0.0075f;
		float dist = length(light.lightDir - fPosEye.xyz);
		float att = 1.0f / (constant + linear * dist + quadratic * (dist * dist));
		ambient = att * light.ambientStrength * light.lightColor;
		diffuse = att * max(dot(normalEye, lightDirN), 0.0f) * light.lightColor;
		specular = att * light.specularStrength * specCoeff * light.lightColor;
	}
	else {
		ambient = light.ambientStrength * light.lightColor;
		diffuse = max(dot(normalEye, lightDirN), 0.0f) * light.lightColor;
		specular = light.specularStrength * specCoeff * light.lightColor;
	}
	
	
	ambient *= texture(diffuseTexture, fTexCoords).rgb;
	diffuse *= texture(diffuseTexture, fTexCoords).rgb;
	specular *= texture(specularTexture, fTexCoords).rgb;

	//if(colorFromTexture.a < 0.3f) {
	//	discard; //texture discarding
	//}

	return min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);
}

void main() 
{
	mainLight = LightStruct(0.5f, 0.5f, 32.0f, mainLightColor, mainLightDir, mainFragPosLightSpace);
	secondaryLight = LightStruct(0.5f, 0.5f, 32.0f, secondaryLightColor, secondaryLightDir, vec4(1.0f,1.0f,1.0f,1.0f));

	float fogFactor = computeFog();
	vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f); //fog
	
	vec3 color = computeLightComponents(mainLight, true, false) + computeLightComponents(secondaryLight, false, true); //+ computeLightComponents(secondaryLight);
    
    fColor = mix(fogColor, vec4(color, 0.3f), fogFactor); //Pt transparenta se citeste valoarea transparentei din textura
}
