#version 120
attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aTexCoord; // 纹理坐标
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;
varying vec3 FragPos;
varying vec3 Normal;
varying vec2 TexCoord; // 传递纹理坐标到片段着色器

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    TexCoord = aTexCoord; // 传递纹理坐标
    gl_Position = projection * view * vec4(FragPos, 1.0);
}