#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying vec2 TexCoord; // 接收纹理坐标
uniform sampler2D textureSampler; // 纹理采样器

void main() {
    // 应用纹理
    vec4 texColor = texture2D(textureSampler, TexCoord);
    
    // 简单光照
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.2);
    
    // 混合纹理颜色和光照
    vec3 result = texColor.rgb * diff;
    gl_FragColor = vec4(result, 1.0);
}