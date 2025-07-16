#version 120
attribute vec3 aPos;
attribute vec3 aNormal;
attribute float aCurvature;  // 添加曲率属性
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
varying vec3 FragPos;
varying vec3 Normal;
varying float Curvature;  // 传递曲率到片段着色器

void main() {
   FragPos = vec3(model * vec4(aPos, 1.0));
   Normal = normalMatrix * aNormal;
   Curvature = aCurvature;  // 传递曲率值
   gl_Position = projection * view * vec4(FragPos, 1.0);
}