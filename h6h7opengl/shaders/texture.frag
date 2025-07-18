#version 120
varying vec2 TexCoord; // 接收纹理坐标
uniform sampler2D textureSampler; // 纹理采样器

void main() {
    // 应用纹理
    vec4 texColor = texture2D(textureSampler, TexCoord);
    
    // 直接输出纹理颜色
    gl_FragColor = texColor;
}