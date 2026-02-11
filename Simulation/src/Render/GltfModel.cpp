#include "GltfModel.h"
#include "Shader.h"

#include "Campfire/tiny_gltf.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace {

    using namespace tinygltf;

    bool readAttributeFloats(const Model& model, int accIndex, std::vector<float>& out) {
        if (accIndex < 0 || size_t(accIndex) >= model.accessors.size()) return false;
        const Accessor& acc = model.accessors[size_t(accIndex)];
        if (acc.bufferView < 0 || size_t(acc.bufferView) >= model.bufferViews.size()) return false;
        const BufferView& bv = model.bufferViews[size_t(acc.bufferView)];
        if (bv.buffer < 0 || size_t(bv.buffer) >= model.buffers.size()) return false;
        const Buffer& buf = model.buffers[size_t(bv.buffer)];

        int stride = acc.ByteStride(bv);
        if (stride <= 0) return false;
        int compSize = GetComponentSizeInBytes(static_cast<uint32_t>(acc.componentType));
        int numComp = GetNumComponentsInType(static_cast<uint32_t>(acc.type));
        if (compSize <= 0 || numComp <= 0) return false;

        size_t need = bv.byteOffset + acc.byteOffset + acc.count * size_t(stride);
        if (need > buf.data.size()) return false;

        out.resize(acc.count * size_t(numComp));
        const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;

        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            for (size_t i = 0; i < acc.count; ++i) {
                const float* src = reinterpret_cast<const float*>(base + i * size_t(stride));
                for (int c = 0; c < numComp; ++c)
                    out[i * size_t(numComp) + size_t(c)] = src[c];
            }
            return true;
        }
        if (acc.normalized && acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            for (size_t i = 0; i < acc.count; ++i) {
                const uint16_t* src = reinterpret_cast<const uint16_t*>(base + i * size_t(stride));
                for (int c = 0; c < numComp; ++c)
                    out[i * size_t(numComp) + size_t(c)] = src[c] / 65535.0f;
            }
            return true;
        }
        return false;
    }

    bool readIndices(const Model& model, int accIndex, std::vector<uint32_t>& out) {
        if (accIndex < 0 || size_t(accIndex) >= model.accessors.size()) return false;
        const Accessor& acc = model.accessors[size_t(accIndex)];
        if (acc.bufferView < 0 || size_t(acc.bufferView) >= model.bufferViews.size()) return false;
        const BufferView& bv = model.bufferViews[size_t(acc.bufferView)];
        if (bv.buffer < 0 || size_t(bv.buffer) >= model.buffers.size()) return false;
        const Buffer& buf = model.buffers[size_t(bv.buffer)];

        int stride = acc.ByteStride(bv);
        if (stride <= 0) return false;
        size_t need = bv.byteOffset + acc.byteOffset + acc.count * size_t(stride);
        if (need > buf.data.size()) return false;

        out.resize(acc.count);
        const uint8_t* base = buf.data.data() + bv.byteOffset + acc.byteOffset;

        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            for (size_t i = 0; i < acc.count; ++i)
                out[i] = *reinterpret_cast<const uint32_t*>(base + i * size_t(stride));
            return true;
        }
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            for (size_t i = 0; i < acc.count; ++i)
                out[i] = *reinterpret_cast<const uint16_t*>(base + i * size_t(stride));
            return true;
        }
        return false;
    }

    GLuint createTextureFromImage(const tinygltf::Image& img) {
        if (img.image.empty() || img.width <= 0 || img.height <= 0) return 0;
        GLenum fmt = GL_RGBA;
        if (img.component == 3) fmt = GL_RGB;
        else if (img.component != 4) return 0;

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, fmt, img.width, img.height, 0, fmt, GL_UNSIGNED_BYTE, img.image.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

} // namespace

GltfModel::~GltfModel() {
    cleanup();
}

void GltfModel::cleanup() {
    for (auto& p : m_parts) {
        if (p.texture) { glDeleteTextures(1, &p.texture); p.texture = 0; }
        if (p.ebo) { glDeleteBuffers(1, &p.ebo); p.ebo = 0; }
        if (p.vbo) { glDeleteBuffers(1, &p.vbo); p.vbo = 0; }
        if (p.vao) { glDeleteVertexArrays(1, &p.vao); p.vao = 0; }
    }
    m_parts.clear();
    m_loaded = false;
}

bool GltfModel::loadFromFile(const std::string& path) {
    cleanup();

    TinyGLTF loader;
    Model model;
    std::string err, warn;
    bool ok = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (!warn.empty()) std::cerr << "GltfModel warn: " << warn << std::endl;
    if (!ok) {
        std::cerr << "GltfModel: " << err << std::endl;
        return false;
    }

    if (model.meshes.empty()) {
        std::cerr << "GltfModel: no meshes " << path << std::endl;
        return false;
    }

    for (const auto& mesh : model.meshes) {
        for (const auto& prim : mesh.primitives) {
            auto itPos = prim.attributes.find("POSITION");
            if (itPos == prim.attributes.end()) continue;

            std::vector<float> posF, nrmF, uvF;
            if (!readAttributeFloats(model, itPos->second, posF) || posF.size() % 3 != 0) continue;
            size_t vCount = posF.size() / 3;

            auto itNrm = prim.attributes.find("NORMAL");
            if (itNrm != prim.attributes.end())
                readAttributeFloats(model, itNrm->second, nrmF);
            auto itUv = prim.attributes.find("TEXCOORD_0");
            if (itUv != prim.attributes.end())
                readAttributeFloats(model, itUv->second, uvF);

            std::vector<Vertex> vertices(vCount);
            for (size_t i = 0; i < vCount; ++i) {
                vertices[i].pos = glm::vec3(posF[i * 3], posF[i * 3 + 1], posF[i * 3 + 2]);
                vertices[i].nrm = (nrmF.size() >= (i + 1) * 3)
                    ? glm::vec3(nrmF[i * 3], nrmF[i * 3 + 1], nrmF[i * 3 + 2])
                    : glm::vec3(0.f, 1.f, 0.f);
                vertices[i].uv = (uvF.size() >= (i + 1) * 2)
                    ? glm::vec2(uvF[i * 2], uvF[i * 2 + 1])
                    : glm::vec2(0.f, 0.f);
            }

            std::vector<uint32_t> indices;
            if (prim.indices >= 0)
                readIndices(model, prim.indices, indices);
            if (indices.empty()) {
                indices.resize(vCount);
                for (size_t i = 0; i < vCount; ++i) indices[i] = static_cast<uint32_t>(i);
            }

            glm::vec3 baseColor(0.6f, 0.35f, 0.2f);
            GLuint texId = 0;
            if (prim.material >= 0 && size_t(prim.material) < model.materials.size()) {
                const auto& mat = model.materials[size_t(prim.material)];
                const auto& pbr = mat.pbrMetallicRoughness;
                if (pbr.baseColorFactor.size() >= 3) {
                    baseColor.r = static_cast<float>(pbr.baseColorFactor[0]);
                    baseColor.g = static_cast<float>(pbr.baseColorFactor[1]);
                    baseColor.b = static_cast<float>(pbr.baseColorFactor[2]);
                }
                if (pbr.baseColorTexture.index >= 0 && size_t(pbr.baseColorTexture.index) < model.textures.size()) {
                    int src = model.textures[size_t(pbr.baseColorTexture.index)].source;
                    if (src >= 0 && size_t(src) < model.images.size())
                        texId = createTextureFromImage(model.images[size_t(src)]);
                }
            }

            Part part;
            part.baseColor = baseColor;
            part.texture = texId;
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
            shader->setInt("uUseTexture", p.texture ? 1 : 0);
            if (p.texture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, p.texture);
                shader->setInt("uTexture", 0);
            }
        }
        glBindVertexArray(p.vao);
        glDrawElements(GL_TRIANGLES, p.indexCount, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
        if (p.texture) glBindTexture(GL_TEXTURE_2D, 0);
    }
}
