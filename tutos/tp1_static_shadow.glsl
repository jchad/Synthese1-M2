#version 430

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec3 normal;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMaxtrix;
uniform mat4 invViewMatrix;
uniform vec4 lightPos;

out vec3 n;
out vec3 l;
out vec3 v;

void main( )
{
    vec3 p= position;
    gl_Position= mvpMatrix * vec4(p, 1);


    n = mat3(modelMatrix) * normal;
    l = vec3(lightPos) - vec3(modelMatrix * vec4(p, 1));
    v = vec3(invViewMatrix * vec4(0,0,0,1)) - vec3(modelMatrix * vec4(p, 1));
}

#endif
