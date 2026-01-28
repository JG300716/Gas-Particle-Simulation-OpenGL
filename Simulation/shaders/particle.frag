#version 460 core
out vec4 FragColor;

in float vDensity;

void main() {
    // Na razie nic nie modyfikujemy - po prostu renderujemy białe punkty
    FragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
