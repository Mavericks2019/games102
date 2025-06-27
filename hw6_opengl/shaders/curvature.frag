#version 120
varying vec3 FragPos;
varying vec3 Normal;
uniform int curvatureType;

vec3 mapToColor(float value) {
    // 蓝色 (低曲率) -> 青色 -> 绿色 -> 黄色 -> 红色 (高曲率)
    if (value < 0.25) {
        return vec3(0.0, value * 4.0, 1.0);
    } else if (value < 0.5) {
        return vec3(0.0, 1.0, 1.0 - (value - 0.25) * 4.0);
    } else if (value < 0.75) {
        return vec3((value - 0.5) * 4.0, 1.0, 0.0);
    } else {
        return vec3(1.0, 1.0 - (value - 0.75) * 4.0, 0.0);
    }
}

void main() {
    // 基于法线计算简单曲率
    vec3 norm = normalize(Normal);
    
    // 计算法线在屏幕空间的变化率
    vec3 dndx = dFdx(norm);
    vec3 dndy = dFdy(norm);
    
    float curvature = length(dndx) + length(dndy);
    
    // 应用非线性缩放增强视觉效果
    curvature = pow(curvature * 5.0, 0.7);
    curvature = clamp(curvature, 0.0, 1.0);
    
    // 映射到颜色
    vec3 color = mapToColor(curvature);
    gl_FragColor = vec4(color, 1.0);
}