#version 120
varying vec2 TexCoord; // 接收纹理坐标
uniform sampler2D textureSampler; // 纹理采样器
uniform bool isParameterizationView; // 新增：是否参数化视图

void main() {
    // 应用纹理
    vec4 texColor = texture2D(textureSampler, TexCoord);
    
    // 如果是参数化视图（右侧），直接输出纹理颜色，无光照
    if (isParameterizationView) {
        gl_FragColor = texColor;
    } else {
        // 左侧视图保留简单光照效果
        vec3 color = texColor.rgb;
        gl_FragColor = vec4(color, 1.0);
    }
}