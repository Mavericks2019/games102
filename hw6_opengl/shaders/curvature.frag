#version 120
varying vec3 FragPos;
varying vec3 Normal;
varying float Curvature;  // 接收插值后的曲率值
uniform int curvatureType;

vec3 mapToColor(float c) {
    // 确保曲率值在[0,1]范围内
    c = clamp(c, 0.0, 1.0);
    
    // 分段线性插值颜色映射
    // [0, 1/8]: 深蓝到蓝
    // [1/8, 3/8]: 蓝到青
    // [3/8, 5/8]: 青到黄
    // [5/8, 7/8]: 黄到红
    // [7/8, 1]: 红到深红
    
    float r, g, b;
    
    if (c < 0.125) { // 0-1/8
        r = 0.0;
        g = 0.0;
        b = 0.5 + c / 0.125 * 0.5; // 0.5 -> 1.0
    } 
    else if (c < 0.375) { // 1/8-3/8
        r = 0.0;
        g = (c - 0.125) / 0.25; // 0.0 -> 1.0
        b = 1.0;
    } 
    else if (c < 0.625) { // 3/8-5/8
        r = (c - 0.375) / 0.25; // 0.0 -> 1.0
        g = 1.0;
        b = 1.0 - (c - 0.375) / 0.25; // 1.0 -> 0.0
    } 
    else if (c < 0.875) { // 5/8-7/8
        r = 1.0;
        g = 1.0 - (c - 0.625) / 0.25; // 1.0 -> 0.0
        b = 0.0;
    } 
    else { // 7/8-1
        r = 1.0 - (c - 0.875) / 0.125 * 0.5; // 1.0 -> 0.5
        g = 0.0;
        b = 0.0;
    }
    
    return vec3(r, g, b);
}

void main() {
    // 使用插值后的曲率值
    float curvature = Curvature;
    
    // 映射到颜色
    vec3 color = mapToColor(curvature);
    gl_FragColor = vec4(color, 1.0);
}