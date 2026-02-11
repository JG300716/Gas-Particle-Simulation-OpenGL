#version 460 core
in vec3 vNormal;
in vec2 vTexCoord;

out vec4 FragColor;

uniform vec3 uColor;
uniform bool uUseTexture;
uniform sampler2D uTexture;
uniform vec3 uLightDir;

void main() {
    vec3 n = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);
    float d = max(0.0, dot(n, lightDir));
    float lit = 0.3 + 0.7 * d;  // 30% ambient + 70% diffuse
    vec3 col;
    if (uUseTexture) {
        col = texture(uTexture, vTexCoord).rgb * uColor * lit;
    } else {
        col = uColor * lit;
    }
    FragColor = vec4(col, 1.0);
}
