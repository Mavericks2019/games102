#version 120
uniform vec4 lineColor; // 添加颜色uniform变量
void main() {
   gl_FragColor = lineColor; // 使用uniform变量设置颜色
}