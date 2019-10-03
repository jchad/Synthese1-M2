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
layout(location= 2) in vec3 position2;
layout(location= 3) in vec3 normal2;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 viewMaxtrix;
uniform mat4 invViewMatrix;
uniform vec4 lightPos;
uniform float dt;
uniform mat4 sourceMatrix;

out vec3 n;
out vec3 l;
out vec3 v;
out vec3 p;

void main( )
{
    vec3 pos= position * (1-dt) + position2*dt;
    gl_Position= mvpMatrix * vec4(pos, 1);

    vec3 norm = normal * (1-dt) + normal2*dt;

    n = mat3(modelMatrix) * norm;
    l = vec3(lightPos) - vec3(modelMatrix * vec4(pos, 1));
    v = vec3(invViewMatrix * vec4(0,0,0,1)) - vec3(modelMatrix * vec4(pos, 1));
    p = vec3(sourceMatrix * vec4(pos + (0.1 * norm),1));
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
    float depth = texture(shadowMap, p.xy).x;

    float shadow = 1.f;

//    vec2 poissonDisk[4] = vec2[](
//      vec2( -0.94201624, -0.39906216 ),
//      vec2( 0.94558609, -0.76890725 ),
//      vec2( -0.094184101, -0.92938870 ),
//      vec2( 0.34495938, 0.29387760 )
//    );

//    for (int i=0;i<4;i++){
//      if ( texture( shadowMap, p.xy + poissonDisk[i]/700.0 ).z  <  p.z ){
//        shadow -=0.2f;
//      }
//    }


    if (depth < p.z) {
        shadow = 0.2f;
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

    vec3 result = ambient + diffuse + specular * diff;

    fragment_color= vec4(shadow * result,1);
}

#endif
