#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying float vCurvature;   // 接收插值的曲率值

uniform int curvatureType;

vec3 mapToColor(float t) {
    // 更平滑的过渡：蓝色(0) -> 青色 -> 绿色 -> 黄色 -> 红色(1)
    vec3 color;
    
    if (t < 0.25) {
        // 蓝色到青色：增加绿色分量
        color = vec3(0.0, t * 4.0, 1.0);
    } else if (t < 0.5) {
        // 青色到绿色：减少蓝色分量，保持绿色
        color = vec3(0.0, 1.0, 1.0 - (t - 0.25) * 4.0);
    } else if (t < 0.75) {
        // 绿色到黄色：增加红色分量
        color = vec3((t - 0.5) * 4.0, 1.0, 0.0);
    } else {
        // 黄色到红色：减少绿色分量
        color = vec3(1.0, 1.0 - (t - 0.75) * 4.0, 0.0);
    }
    
    return color;
}

void main() {
    // 使用插值的曲率值
    float curvature = vCurvature;
    
    // 应用非线性缩放增强视觉效果
    curvature = pow(curvature, 0.7); // 移除乘以5.0，只应用幂函数
    
    // 确保在[0,1]范围内
    curvature = clamp(curvature, 0.0, 1.0);
    
    // 映射到颜色
    vec3 color = mapToColor(curvature);
    gl_FragColor = vec4(color, 1.0);
}