#version 410 core

in vec3 fNormal;
in vec4 fPosEye;

in vec3 reflection;

out vec4 fColor;


//texture
uniform samplerCube skybox;

void main() 
{
	vec4 colorFromSkybox = texture(skybox, reflection);

    fColor = colorFromSkybox;
}
