#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in float aCurvature;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
out vec3 FragPos;
out vec3 Normal;
out float Curvature;

void main() {
   FragPos = vec3(model * vec4(aPos, 1.0));
   Normal = normalMatrix * aNormal;
   Curvature = aCurvature;
   gl_Position = projection * view * vec4(FragPos, 1.0);
}