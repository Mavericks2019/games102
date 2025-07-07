#version 120
varying vec3 FragPos;
varying vec3 Normal;

void main() {
    // 使用法线作为基础颜色
    vec3 norm = normalize(Normal);
    vec3 color = (norm + vec3(1.0)) * 0.5;
    
    // 简单光照
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    float diff = max(dot(norm, lightDir), 0.2);
    
    gl_FragColor = vec4(color * diff, 1.0);
}