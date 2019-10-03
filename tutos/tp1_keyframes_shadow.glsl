#version 430
#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec3 normal;
layout(location= 2) in vec3 position2;
layout(location= 3) in vec3 normal2;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMaxtrix;
uniform mat4 invViewMatrix;
uniform vec4 lightPos;
uniform float dt;

out vec3 n;
out vec3 l;
out vec3 v;

void main( )
{
    vec3 p= position * (1-dt) + position2*dt;
    gl_Position= mvpMatrix * vec4(p, 1);

    vec3 norm = normal * (1-dt) + normal2*dt;

    n = mat3(modelMatrix) * norm;
    l = vec3(lightPos) - vec3(modelMatrix * vec4(p, 1));
    v = vec3(invViewMatrix * vec4(0,0,0,1)) - vec3(modelMatrix * vec4(p, 1));
}

#endif
