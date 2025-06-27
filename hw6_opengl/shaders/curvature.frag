#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying float vCurvature;   // 接收插值的曲率值

uniform int curvatureType;

vec3 mapToColor(float value) {
    // 使用平滑的颜色过渡
    vec3 lowColor = vec3(0.0, 0.0, 1.0);    // 蓝色 (低曲率)
    vec3 midColor = vec3(0.0, 1.0, 0.0);    // 绿色 (中曲率)
    vec3 highColor = vec3(1.0, 0.0, 0.0);   // 红色 (高曲率)
    
    if (value < 0.5) {
        float t = value * 2.0;
        return mix(lowColor, midColor, t);
    } else {
        float t = (value - 0.5) * 2.0;
        return mix(midColor, highColor, t);
    }
}

void main() {
    // 使用插值的曲率值
    float curvature = vCurvature;
    
    // 应用非线性缩放增强视觉效果
    curvature = pow(curvature * 5.0, 0.7);
    curvature = clamp(curvature, 0.0, 1.0);
    
    // 映射到颜色
    vec3 color = mapToColor(curvature);
    gl_FragColor = vec4(color, 1.0);
}