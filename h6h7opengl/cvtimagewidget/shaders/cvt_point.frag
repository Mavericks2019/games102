#version 330 core
out vec4 FragColor;
void main() {
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord) * 2.0;
    if (dist > 1.0) discard;
    if (dist < 0.8) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
}