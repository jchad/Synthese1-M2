#version 430
struct material{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};

layout(std430, binding = 0) readonly buffer materialsData
{
    material data[];
};
#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec3 normal;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMaxtrix;
uniform mat4 invViewMatrix;
uniform vec4 lightPos;
uniform mat4 sourceMatrix;

out vec3 n;
out vec3 l;
out vec3 v;
out vec3 p;

void main( )
{
    vec3 pos= position;
    gl_Position= mvpMatrix * vec4(pos, 1);


    n = mat3(modelMatrix) * normal;
    l = vec3(lightPos) - vec3(modelMatrix * vec4(pos, 1));
    v = vec3(invViewMatrix * vec4(0,0,0,1)) - vec3(modelMatrix * vec4(pos, 1));
    p = vec3(sourceMatrix * vec4(pos + (0.1 * normal),1));
}

#endif


#ifdef FRAGMENT_SHADER
out vec4 fragment_color;
uniform vec4 lightColor;
uniform sampler2D shadowMap;
in vec3 n;
in vec3 l;
in vec3 v;
in vec3 p;

void main( )
{
//    fragment_color= vec4(p.xy, 0, 1);
//            return;

    float depth = texture(shadowMap, p.xy).x;

    float shadow;

    if (depth < p.z) {
        shadow = 0.2f;
    } else {
        shadow = 1.f;
    }

    vec3 N = normalize(n);
    vec3 L = normalize(l);
    vec3 V = normalize(v);
    vec3 ambient = vec3(lightColor * data[gl_PrimitiveID].ambient);

    float diff = max(0, dot(N,L));
    vec3 diffuse = vec3(lightColor * (diff * data[gl_PrimitiveID].diffuse));

    //vec3 reflectDir = reflect(-L,N);

    vec3 H = normalize(L+V);

    float cosTheta = dot(N, H);

    float coeff = (data[gl_PrimitiveID].shininess + 8) / (8 * 3.14);

    float spec = coeff * pow(max(cosTheta, 0.0), data[gl_PrimitiveID].shininess);
    //float spec = pow(max(dot(reflectDir, V), 0.0), data[gl_PrimitiveID].shininess);
    vec3 specular = vec3(lightColor * (spec * data[gl_PrimitiveID].specular));

    vec3 result = diffuse + specular * diff;

    fragment_color= vec4(shadow * result,1);
}

#endif
