#version 330 core
uniform float iTime;
uniform vec2 iResolution;
uniform vec2 iMouse;

in vec2 fragCoord;
out vec4 fragColor;

void main()
{
    vec2 uv = fragCoord;
    
    // 创建随时间变化的颜色
    vec3 color = vec3(
        abs(sin(iTime * 0.5)), 
        abs(cos(iTime * 0.3)), 
        abs(sin(iTime * 0.7))
    );
    
    // 添加鼠标交互效果
    float dist = distance(uv, iMouse);
    color *= smoothstep(0.2, 0.1, dist);
    
    fragColor = vec4(color, 1.0);
}