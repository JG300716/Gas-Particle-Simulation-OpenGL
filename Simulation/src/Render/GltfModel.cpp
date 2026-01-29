#include "GltfModel.h"
#include "Shader.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

static const uint32_t GLB_MAGIC = 0x46546C67;
static const uint32_t GLB_JSON_CHUNK = 0x4E4F534A;
static const uint32_t GLB_BIN_CHUNK = 0x004E4942;

struct AccessorInfo {
    int bufferView = -1;
    int byteOffset = 0;
    int componentType = 5126;
    int count = 0;
    std::string type;
};

struct BufferViewInfo {
    int byteOffset = 0;
    int byteLength = 0;
};

int findJsonInt(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return -1;
    pos += pat.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size()) return -1;
    const char* p = json.c_str() + pos;
    if ((*p != '-' && !std::isdigit(static_cast<unsigned char>(*p)))) return -1;
    return std::atoi(p);
}

float parseFloat(const char* str, const char** end) {
    while (*str == ' ' || *str == '\t') ++str;
    char* e = nullptr;
    float v = std::strtof(str, &e);
    if (end) *end = e ? static_cast<const char*>(e) : str;
    return v;
}

glm::vec3 extractBaseColorFactor(const std::string& matObj) {
    glm::vec3 c(0.6f, 0.35f, 0.2f);
    size_t pbr = matObj.find("\"pbrMetallicRoughness\"");
    if (pbr == std::string::npos) return c;
    size_t start = matObj.find('{', pbr);
    if (start == std::string::npos) return c;
    int depth = 1;
    size_t end = start + 1;
    while (end < matObj.size() && depth > 0) {
        if (matObj[end] == '{') ++depth;
        else if (matObj[end] == '}') --depth;
        ++end;
    }
    std::string pbrObj = matObj.substr(start, end - start);
    std::string pat = "\"baseColorFactor\":";
    size_t pos = pbrObj.find(pat);
    if (pos == std::string::npos) return c;
    pos += pat.size();
    while (pos < pbrObj.size() && (pbrObj[pos] == ' ' || pbrObj[pos] == '\t')) ++pos;
    if (pos >= pbrObj.size() || pbrObj[pos] != '[') return c;
    ++pos;
    const char* p = pbrObj.c_str() + pos;
    const char* e = nullptr;
    float v[4] = { 1, 1, 1, 1 };
    for (int i = 0; i < 4 && *p && *p != ']'; ++i) {
        v[i] = parseFloat(p, &e);
        p = e;
        while (*p == ',' || *p == ' ' || *p == '\t') ++p;
    }
    c.r = v[0]; c.g = v[1]; c.b = v[2];
    return c;
}

std::string findJsonString(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\":\"";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return "";
    pos += pat.size();
    size_t end = pos;
    while (end < json.size() && json[end] != '"') ++end;
    if (end >= json.size()) return "";
    return json.substr(pos, end - pos);
}

bool extractAccessor(const std::string& obj, AccessorInfo& out) {
    out.bufferView = findJsonInt(obj, "bufferView");
    out.byteOffset = findJsonInt(obj, "byteOffset");
    if (out.byteOffset < 0) out.byteOffset = 0;
    int ct = findJsonInt(obj, "componentType");
    if (ct >= 0) out.componentType = ct;
    out.count = findJsonInt(obj, "count");
    out.type = findJsonString(obj, "type");
    return out.bufferView >= 0 && out.count > 0 && !out.type.empty();
}

bool extractBufferView(const std::string& obj, BufferViewInfo& out) {
    out.byteOffset = findJsonInt(obj, "byteOffset");
    if (out.byteOffset < 0) out.byteOffset = 0;
    out.byteLength = findJsonInt(obj, "byteLength");
    return out.byteLength > 0;
}

bool extractNthObjectFromArray(const std::string& arr, size_t index, std::string& out) {
    size_t i = 0;
    int depth = 0;
    bool inObj = false;
    size_t start = 0;
    for (size_t k = 0; k < arr.size(); ++k) {
        char c = arr[k];
        if (c == '{') {
            if (depth == 0) { start = k; inObj = true; }
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0 && inObj) {
                if (i == index) {
                    out = arr.substr(start, k - start + 1);
                    return true;
                }
                ++i;
                inObj = false;
            }
        }
    }
    return false;
}

std::string extractArray(const std::string& json, const std::string& key) {
    std::string pat = "\"" + key + "\":";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return "";
    pos += pat.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    if (pos >= json.size() || json[pos] != '[') return "";
    size_t start = pos;
    int depth = 1;
    ++pos;
    while (pos < json.size() && depth > 0) {
        if (json[pos] == '[') ++depth;
        else if (json[pos] == ']') --depth;
        ++pos;
    }
    if (depth != 0) return "";
    return json.substr(start, pos - start);
}

void collectAllPrimitives(const std::string& json, std::vector<std::string>& prims, std::vector<int>& matIndices) {
    std::string meshArr = extractArray(json, "meshes");
    if (meshArr.empty()) return;
    for (size_t mi = 0; ; ++mi) {
        std::string meshObj;
        if (!extractNthObjectFromArray(meshArr, mi, meshObj)) break;
        std::string primArr = extractArray(meshObj, "primitives");
        if (primArr.empty()) continue;
        for (size_t pi = 0; ; ++pi) {
            std::string primObj;
            if (!extractNthObjectFromArray(primArr, pi, primObj)) break;
            prims.push_back(primObj);
            int mat = findJsonInt(primObj, "material");
            matIndices.push_back(mat >= 0 ? mat : -1);
        }
    }
}

} // namespace

GltfModel::~GltfModel() {
    cleanup();
}

void GltfModel::cleanup() {
    for (auto& p : m_parts) {
        if (p.ebo) { glDeleteBuffers(1, &p.ebo); p.ebo = 0; }
        if (p.vbo) { glDeleteBuffers(1, &p.vbo); p.vbo = 0; }
        if (p.vao) { glDeleteVertexArrays(1, &p.vao); p.vao = 0; }
    }
    m_parts.clear();
    m_loaded = false;
}

bool GltfModel::loadFromFile(const std::string& path) {
    cleanup();

    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        std::cerr << "GltfModel: cannot open " << path << std::endl;
        return false;
    }
    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);
    std::vector<char> buf(static_cast<size_t>(size));
    if (!f.read(buf.data(), size)) {
        std::cerr << "GltfModel: read failed " << path << std::endl;
        return false;
    }
    const uint8_t* base = reinterpret_cast<const uint8_t*>(buf.data());

    if (size < 12) return false;
    uint32_t magic, version, totalLen;
    memcpy(&magic, base, 4);
    memcpy(&version, base + 4, 4);
    memcpy(&totalLen, base + 8, 4);
    if (magic != GLB_MAGIC || version != 2) {
        std::cerr << "GltfModel: invalid GLB " << path << std::endl;
        return false;
    }

    size_t off = 12;
    const uint8_t* jsonData = nullptr;
    size_t jsonLen = 0;
    const uint8_t* binData = nullptr;
    size_t binLen = 0;

    while (off + 8 <= static_cast<size_t>(size)) {
        uint32_t chunkLen, chunkType;
        memcpy(&chunkLen, base + off, 4);
        memcpy(&chunkType, base + off + 4, 4);
        off += 8;
        if (off + chunkLen > static_cast<size_t>(size)) break;
        if (chunkType == GLB_JSON_CHUNK) { jsonData = base + off; jsonLen = chunkLen; }
        else if (chunkType == GLB_BIN_CHUNK) { binData = base + off; binLen = chunkLen; }
        off += (chunkLen + 3) & ~3u;
    }

    if (!jsonData || !jsonLen || !binData || !binLen) {
        std::cerr << "GltfModel: no JSON/BIN " << path << std::endl;
        return false;
    }
    std::string json(jsonData, jsonData + jsonLen);

    std::string accArr = extractArray(json, "accessors");
    std::string bvArr = extractArray(json, "bufferViews");
    std::string matArr = extractArray(json, "materials");
    if (accArr.empty() || bvArr.empty()) {
        std::cerr << "GltfModel: no accessors/bufferViews " << path << std::endl;
        return false;
    }

    std::vector<std::string> prims;
    std::vector<int> matIndices;
    collectAllPrimitives(json, prims, matIndices);
    if (prims.empty()) {
        std::cerr << "GltfModel: no primitives " << path << std::endl;
        return false;
    }

    std::vector<BufferViewInfo> bvs;
    std::string obj;
    for (size_t i = 0; ; ++i) {
        if (!extractNthObjectFromArray(bvArr, i, obj)) break;
        BufferViewInfo bv;
        if (extractBufferView(obj, bv)) bvs.push_back(bv);
    }

    auto copyFromBin = [&](const AccessorInfo& a, const BufferViewInfo& bv) -> std::vector<float> {
        int comp = (a.type == "VEC3") ? 3 : (a.type == "VEC2") ? 2 : 1;
        size_t o = static_cast<size_t>(bv.byteOffset + a.byteOffset);
        size_t n = static_cast<size_t>(a.count) * static_cast<size_t>(comp);
        std::vector<float> out(n);
        if (a.componentType == 5126) {
            if (o + n * 4 > binLen) return {};
            const float* src = reinterpret_cast<const float*>(binData + o);
            for (size_t i = 0; i < n; ++i) out[i] = src[i];
        } else {
            if (o + n * 2 > binLen) return {};
            const uint16_t* src = reinterpret_cast<const uint16_t*>(binData + o);
            for (size_t i = 0; i < n; ++i) out[i] = src[i] / 65535.0f;
        }
        return out;
    };

    auto copyIndicesFromBin = [&](const AccessorInfo& a, const BufferViewInfo& bv) -> std::vector<uint32_t> {
        size_t o = static_cast<size_t>(bv.byteOffset + a.byteOffset);
        size_t n = static_cast<size_t>(a.count);
        std::vector<uint32_t> out(n);
        if (a.componentType == 5125) {
            if (o + n * 4 > binLen) return {};
            memcpy(out.data(), binData + o, n * 4);
        } else if (a.componentType == 5123) {
            if (o + n * 2 > binLen) return {};
            const uint16_t* src = reinterpret_cast<const uint16_t*>(binData + o);
            for (size_t i = 0; i < n; ++i) out[i] = src[i];
        } else return {};
        return out;
    };

    for (size_t primIdx = 0; primIdx < prims.size(); ++primIdx) {
        const std::string& primObj = prims[primIdx];
        int posAcc = -1, nrmAcc = -1, uvAcc = -1, idxAcc = -1;
        idxAcc = findJsonInt(primObj, "indices");
        size_t ap = primObj.find("\"attributes\"");
        if (ap != std::string::npos) {
            std::string attObj = primObj.substr(ap);
            size_t br = attObj.find('{');
            if (br != std::string::npos) {
                size_t be = br, d = 1;
                ++be;
                while (be < attObj.size() && d > 0) {
                    if (attObj[be] == '{') ++d;
                    else if (attObj[be] == '}') --d;
                    ++be;
                }
                attObj = attObj.substr(br, be - br);
                posAcc = findJsonInt(attObj, "POSITION");
                nrmAcc = findJsonInt(attObj, "NORMAL");
                uvAcc = findJsonInt(attObj, "TEXCOORD_0");
            }
        }
        if (posAcc < 0) continue;

        AccessorInfo posInfo, nrmInfo, uvInfo, idxInfo;
        if (!extractNthObjectFromArray(accArr, static_cast<size_t>(posAcc), obj) || !extractAccessor(obj, posInfo)) continue;
        if (nrmAcc >= 0 && extractNthObjectFromArray(accArr, static_cast<size_t>(nrmAcc), obj)) extractAccessor(obj, nrmInfo);
        if (uvAcc >= 0 && extractNthObjectFromArray(accArr, static_cast<size_t>(uvAcc), obj)) extractAccessor(obj, uvInfo);
        if (idxAcc >= 0 && extractNthObjectFromArray(accArr, static_cast<size_t>(idxAcc), obj)) extractAccessor(obj, idxInfo);

        if (posInfo.bufferView < 0 || static_cast<size_t>(posInfo.bufferView) >= bvs.size()) continue;
        std::vector<float> posF = copyFromBin(posInfo, bvs[static_cast<size_t>(posInfo.bufferView)]);
        if (posF.empty() || posInfo.type != "VEC3") continue;

        size_t vCount = static_cast<size_t>(posInfo.count);
        std::vector<Vertex> vertices(vCount);
        for (size_t i = 0; i < vCount; ++i) {
            vertices[i].pos = glm::vec3(posF[i * 3], posF[i * 3 + 1], posF[i * 3 + 2]);
            vertices[i].nrm = glm::vec3(0.0f, 1.0f, 0.0f);
            vertices[i].uv = glm::vec2(0.0f, 0.0f);
        }
        if (nrmInfo.bufferView >= 0 && static_cast<size_t>(nrmInfo.bufferView) < bvs.size()) {
            std::vector<float> nf = copyFromBin(nrmInfo, bvs[static_cast<size_t>(nrmInfo.bufferView)]);
            if (nf.size() >= vCount * 3)
                for (size_t i = 0; i < vCount; ++i)
                    vertices[i].nrm = glm::vec3(nf[i * 3], nf[i * 3 + 1], nf[i * 3 + 2]);
        }
        if (uvInfo.bufferView >= 0 && static_cast<size_t>(uvInfo.bufferView) < bvs.size()) {
            std::vector<float> uf = copyFromBin(uvInfo, bvs[static_cast<size_t>(uvInfo.bufferView)]);
            if (uf.size() >= vCount * 2)
                for (size_t i = 0; i < vCount; ++i)
                    vertices[i].uv = glm::vec2(uf[i * 2], uf[i * 2 + 1]);
        }

        std::vector<uint32_t> indices;
        if (idxInfo.bufferView >= 0 && static_cast<size_t>(idxInfo.bufferView) < bvs.size())
            indices = copyIndicesFromBin(idxInfo, bvs[static_cast<size_t>(idxInfo.bufferView)]);
        if (indices.empty()) {
            indices.resize(vCount);
            for (size_t i = 0; i < vCount; ++i) indices[i] = static_cast<uint32_t>(i);
        }

        glm::vec3 baseColor(0.6f, 0.35f, 0.2f);
        int matIdx = primIdx < matIndices.size() ? matIndices[primIdx] : -1;
        if (matIdx >= 0 && !matArr.empty()) {
            std::string matObj;
            if (extractNthObjectFromArray(matArr, static_cast<size_t>(matIdx), matObj))
                baseColor = extractBaseColorFactor(matObj);
        }

        Part part;
        part.baseColor = baseColor;
        part.indexCount = static_cast<GLsizei>(indices.size());
        glGenVertexArrays(1, &part.vao);
        glGenBuffers(1, &part.vbo);
        glGenBuffers(1, &part.ebo);
        glBindVertexArray(part.vao);
        glBindBuffer(GL_ARRAY_BUFFER, part.vbo);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, part.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nrm));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
        glBindVertexArray(0);

        m_parts.push_back(part);
    }

    if (m_parts.empty()) {
        std::cerr << "GltfModel: no valid primitives " << path << std::endl;
        return false;
    }
    m_loaded = true;
    return true;
}

void GltfModel::draw() const {
    draw(nullptr);
}

void GltfModel::draw(const Shader* shader) const {
    if (!m_loaded) return;
    for (const auto& p : m_parts) {
        if (shader) {
            shader->use();
            shader->setVec3("uColor", p.baseColor);
        }
        glBindVertexArray(p.vao);
        glDrawElements(GL_TRIANGLES, p.indexCount, GL_UNSIGNED_INT, (void*)0);
        glBindVertexArray(0);
    }
}
