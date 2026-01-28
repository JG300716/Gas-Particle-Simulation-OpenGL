#version 460 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in float aSize;
layout (location = 2) in float aDensity;

uniform mat4 uProjection;
uniform mat4 uView;

out float vDensity;

void main() {
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
    gl_PointSize = aSize;
    vDensity = aDensity;
}
