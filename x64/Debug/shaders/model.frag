#version 460 core
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uColor;

void main() {
    vec3 n = normalize(vNormal);
    float d = max(0.0, dot(n, vec3(0.4, 0.8, 0.2)));
    vec3 col = uColor * (0.4 + 0.6 * d);
    FragColor = vec4(col, 1.0);
}
