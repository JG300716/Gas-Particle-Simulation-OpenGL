#version 460 core
in vec2 vUV;

out vec4 FragColor;

uniform mat4 uInvViewProj;
uniform vec3 uCamPos;
uniform vec3 uVolumeMin;
uniform vec3 uVolumeMax;
uniform sampler3D uDensity;
uniform bool uShowTemp;
uniform sampler3D uTemperature;
uniform float uTempMin;
uniform float uTempMax;

const int MAX_STEPS = 128;
const float DENSITY_SCALE = 2.5;

// gradient: 0 dark blue -> 0.5 red -> 1 white
vec3 tempToColor(float t) {
    t = clamp(t, 0.0, 1.0);
    if (t < 0.5) {
        float x = t * 2.0;
        return mix(vec3(0.0, 0.0, 0.25), vec3(0.9, 0.0, 0.0), x);
    } else {
        float x = (t - 0.5) * 2.0;
        return mix(vec3(0.9, 0.0, 0.0), vec3(1.0, 1.0, 1.0), x);
    }
}

bool intersectBox(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax, out float t0, out float t1) {
    vec3 eps = vec3(1e-7);
    vec3 invR = 1.0 / (rd + eps * sign(rd + eps));
    vec3 tbot = invR * (bmin - ro);
    vec3 ttop = invR * (bmax - ro);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    t0 = max(max(tmin.x, tmin.y), tmin.z);
    t1 = min(min(tmax.x, tmax.y), tmax.z);
    return t0 <= t1 && t1 > 0.0;
}

void main() {
    vec4 ndc = vec4(vUV * 2.0 - 1.0, 1.0, 1.0);
    vec4 far = uInvViewProj * ndc;
    vec3 rd = normalize(far.xyz / far.w - uCamPos);

    float t0, t1;
    if (!intersectBox(uCamPos, rd, uVolumeMin, uVolumeMax, t0, t1)) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    t0 = max(0.0, t0);
    float len = t1 - t0;
    float step = len / float(MAX_STEPS);
    vec3 pos = uCamPos + rd * (t0 + 0.5 * step);
    vec3 diag = uVolumeMax - uVolumeMin;
    float stepNorm = 1.0 / float(MAX_STEPS);

    float acc = 0.0;
    float accT = 0.0;
    float weightSum = 0.0;
    for (int i = 0; i < MAX_STEPS; ++i) {
        vec3 uv = (pos - uVolumeMin) / diag;
        if (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0 && uv.z >= 0.0 && uv.z <= 1.0) {
            float d = texture(uDensity, uv).r;
            float w = d * stepNorm * DENSITY_SCALE;
            acc += w;
            if (uShowTemp) {
                float T = texture(uTemperature, uv).r;
                float tr = (uTempMax > uTempMin) ? (T - uTempMin) / (uTempMax - uTempMin) : 0.0;
                accT += tr * w;
                weightSum += w;
            }
        }
        pos += rd * step;
    }

    acc = 1.0 - exp(-acc);
    vec3 col;
    if (uShowTemp && weightSum > 0.0) {
        float tNorm = accT / weightSum;
        col = tempToColor(tNorm);
    } else {
        col = mix(vec3(0.3, 0.3, 0.3), vec3(0.8, 0.8, 0.8), acc);
    }
    FragColor = vec4(col, acc * 0.9);
}
