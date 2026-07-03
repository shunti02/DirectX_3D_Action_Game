#include "Engine/GeometryGenerator.h"
#include <cmath>
#include <unordered_map>

using namespace DirectX;

MeshData GeometryGenerator::CreateMesh(ShapeType type, DirectX::XMFLOAT4 color) {
    switch (type) {
    case ShapeType::CUBE:            return CreateCube(1.0f, color);
    case ShapeType::CAPSULE:         return CreateCapsule(0.5f, 2.0f, 16, color);
    case ShapeType::SPHERE:          return CreateSphere(0.5f, 16, 16, color);
    case ShapeType::TETRAHEDRON:     return CreateTetrahedron(1.0f, color);
    case ShapeType::TORUS:           return CreateTorus(0.8f, 0.2f, 16, 16, color);
    case ShapeType::DOUBLE_PYRAMID:  return CreateDoublePyramid(1.0f, 2.0f, color);
    case ShapeType::CYLINDER:        return CreateCylinder(0.5f, 1.0f, 16, color);
    case ShapeType::CONE:            return CreateCone(0.5f, 1.0f, 16, color);
    case ShapeType::PRISM:           return CreatePrism(1.0f, 1.0f, 1.0f, color);
    case ShapeType::HEXAGONAL_PRISM: return CreateCylinder(0.5f, 1.0f, 6, color);
    case ShapeType::TRUNCATED_CONE:  return CreateTruncatedCone(0.5f, 0.3f, 1.0f, 16, color);
    case ShapeType::WEDGE:           return CreateWedge(1.0f, 1.0f, 1.0f, color);
    default:                         return CreateCube(1.0f, color);
    }
}

void CalculateNormals(MeshData& mesh) {
    for (auto& v : mesh.vertices) {
        v.normal = { 0.0f, 0.0f, 0.0f };
    }
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];

        XMVECTOR v0 = XMLoadFloat3(&mesh.vertices[i0].position);
        XMVECTOR v1 = XMLoadFloat3(&mesh.vertices[i1].position);
        XMVECTOR v2 = XMLoadFloat3(&mesh.vertices[i2].position);

        XMVECTOR e1 = XMVectorSubtract(v1, v0);
        XMVECTOR e2 = XMVectorSubtract(v2, v0);
        XMVECTOR normal = XMVector3Normalize(XMVector3Cross(e1, e2));

        XMFLOAT3 n;
        XMStoreFloat3(&n, normal);

        mesh.vertices[i0].normal.x += n.x; mesh.vertices[i0].normal.y += n.y; mesh.vertices[i0].normal.z += n.z;
        mesh.vertices[i1].normal.x += n.x; mesh.vertices[i1].normal.y += n.y; mesh.vertices[i1].normal.z += n.z;
        mesh.vertices[i2].normal.x += n.x; mesh.vertices[i2].normal.y += n.y; mesh.vertices[i2].normal.z += n.z;
    }

    for (auto& v : mesh.vertices) {
        XMVECTOR n = XMLoadFloat3(&v.normal);
        n = XMVector3Normalize(n);
        XMStoreFloat3(&v.normal, n);
    }
}

MeshData GeometryGenerator::CreateCube(float size, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float h = size * 0.5f;

    XMFLOAT3 positions[] = {
        { -h, h, -h }, { h, h, -h }, { h, h, h }, { -h, h, h },
        { -h, -h, -h }, { h, -h, -h }, { h, -h, h }, { -h, -h, h },
        { -h, -h, h }, { -h, -h, -h }, { -h, h, -h }, { -h, h, h },
        { h, -h, h }, { h, -h, -h }, { h, h, -h }, { h, h, h },
        { -h, -h, -h }, { h, -h, -h }, { h, h, -h }, { -h, h, -h },
        { -h, -h, h }, { h, -h, h }, { h, h, h }, { -h, h, h }
    };

    XMFLOAT3 normals[] = {
        { 0, 1, 0 }, { 0, -1, 0 }, { -1, 0, 0 }, { 1, 0, 0 }, { 0, 0, -1 }, { 0, 0, 1 }
    };

    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 4; ++j) {
            mesh.vertices.push_back({ positions[i * 4 + j].x, positions[i * 4 + j].y, positions[i * 4 + j].z,
                                      normals[i].x, normals[i].y, normals[i].z,
                                      color.x, color.y, color.z, color.w });
        }
        uint32_t offset = i * 4;
        mesh.indices.push_back(offset + 0); mesh.indices.push_back(offset + 1); mesh.indices.push_back(offset + 2);
        mesh.indices.push_back(offset + 0); mesh.indices.push_back(offset + 2); mesh.indices.push_back(offset + 3);
    }
    return mesh;
}

MeshData GeometryGenerator::CreateSphere(float radius, int sliceCount, int stackCount, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    mesh.vertices.push_back({ 0.0f, radius, 0.0f, 0.0f, 1.0f, 0.0f, color.x, color.y, color.z, color.w }); // –k‹É

    float phiStep = XM_PI / stackCount;
    float thetaStep = 2.0f * XM_PI / sliceCount;

    for (int i = 1; i <= stackCount - 1; ++i) {
        float phi = i * phiStep;
        for (int j = 0; j <= sliceCount; ++j) {
            float theta = j * thetaStep;
            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * cosf(phi);
            float z = radius * sinf(phi) * sinf(theta);

            XMVECTOR p = XMVectorSet(x, y, z, 0.0f);
            XMVECTOR n = XMVector3Normalize(p);
            XMFLOAT3 normal; XMStoreFloat3(&normal, n);

            mesh.vertices.push_back({ x, y, z, normal.x, normal.y, normal.z, color.x, color.y, color.z, color.w });
        }
    }
    mesh.vertices.push_back({ 0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, color.x, color.y, color.z, color.w }); // “ì‹É

    for (int i = 1; i <= sliceCount; ++i) {
        mesh.indices.push_back(0); mesh.indices.push_back(i + 1); mesh.indices.push_back(i);
    }
    int baseIndex = 1;
    int ringVertexCount = sliceCount + 1;
    for (int i = 0; i < stackCount - 2; ++i) {
        for (int j = 0; j < sliceCount; ++j) {
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            mesh.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
            mesh.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }
    int southPoleIndex = (int)mesh.vertices.size() - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (int i = 0; i < sliceCount; ++i) {
        mesh.indices.push_back(southPoleIndex); mesh.indices.push_back(baseIndex + i); mesh.indices.push_back(baseIndex + i + 1);
    }
    return mesh;
}

MeshData GeometryGenerator::CreateTruncatedCone(float bottomRadius, float topRadius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float halfHeight = height * 0.5f;

    mesh.vertices.push_back({ 0.0f, halfHeight, 0.0f, 0.0f, 1.0f, 0.0f, color.x, color.y, color.z, color.w }); 
    mesh.vertices.push_back({ 0.0f, -halfHeight, 0.0f, 0.0f, -1.0f, 0.0f, color.x, color.y, color.z, color.w });

    int topCenter = 0, bottomCenter = 1;
    int ringStart = 2;

    for (int i = 0; i <= sliceCount; ++i) {
        float theta = XM_2PI * i / sliceCount;
        float c = cosf(theta), s = sinf(theta);
        mesh.vertices.push_back({ topRadius * c, halfHeight, topRadius * s, 0, 0, 0, color.x, color.y, color.z, color.w });
        mesh.vertices.push_back({ bottomRadius * c, -halfHeight, bottomRadius * s, 0, 0, 0, color.x, color.y, color.z, color.w });
    }

    for (int i = 0; i < sliceCount; ++i) {
        uint32_t t1 = ringStart + i * 2, b1 = t1 + 1;
        uint32_t t2 = ringStart + (i + 1) * 2, b2 = t2 + 1;
        mesh.indices.insert(mesh.indices.end(), { t1, b1, t2, t2, b1, b2 });
        mesh.indices.insert(mesh.indices.end(), { (uint32_t)topCenter, t2, t1 });
        mesh.indices.insert(mesh.indices.end(), { (uint32_t)bottomCenter, b1, b2 });
    }
    CalculateNormals(mesh);
    return mesh;
}

MeshData GeometryGenerator::CreateCylinder(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    return CreateTruncatedCone(radius, radius, height, sliceCount, color);
}

MeshData GeometryGenerator::CreateCone(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    return CreateTruncatedCone(radius, 0.0f, height, sliceCount, color);
}

MeshData GeometryGenerator::CreateDoublePyramid(float size, float height, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float h = height * 0.5f;
    float s = size * 0.5f;

    mesh.vertices = {
        { 0.0f,  h, 0.0f, 0,0,0, color.x, color.y, color.z, color.w },
        { 0.0f, -h, 0.0f, 0,0,0, color.x, color.y, color.z, color.w },
        {  s, 0.0f,  s, 0,0,0, color.x, color.y, color.z, color.w },
        {  s, 0.0f, -s, 0,0,0, color.x, color.y, color.z, color.w },
        { -s, 0.0f, -s, 0,0,0, color.x, color.y, color.z, color.w },
        { -s, 0.0f,  s, 0,0,0, color.x, color.y, color.z, color.w }
    };

    mesh.indices = {
        0, 3, 2,  0, 4, 3,  0, 5, 4,  0, 2, 5,
        1, 2, 3,  1, 3, 4,  1, 4, 5,  1, 5, 2
    };
    CalculateNormals(mesh);
    return mesh;
}

MeshData GeometryGenerator::CreateTorus(float radius, float tubeRadius, int radialSegments, int tubularSegments, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    for (int i = 0; i <= radialSegments; i++) {
        float theta = XM_2PI * (float)i / radialSegments;
        float cosTheta = cosf(theta), sinTheta = sinf(theta);
        for (int j = 0; j <= tubularSegments; j++) {
            float phi = XM_2PI * (float)j / tubularSegments;
            float cosPhi = cosf(phi), sinPhi = sinf(phi);
            float x = (radius + tubeRadius * cosPhi) * cosTheta;
            float z = (radius + tubeRadius * cosPhi) * sinTheta;
            float y = tubeRadius * sinPhi;

            float nx = cosPhi * cosTheta;
            float nz = cosPhi * sinTheta;
            float ny = sinPhi;
            mesh.vertices.push_back({ x, y, z, nx, ny, nz, color.x, color.y, color.z, color.w });
        }
    }
    for (int i = 0; i < radialSegments; i++) {
        for (int j = 0; j < tubularSegments; j++) {
            int current = i * (tubularSegments + 1) + j;
            int next = (i + 1) * (tubularSegments + 1) + j;
            mesh.indices.push_back(current); mesh.indices.push_back(next); mesh.indices.push_back(current + 1);
            mesh.indices.push_back(current + 1); mesh.indices.push_back(next); mesh.indices.push_back(next + 1);
        }
    }
    return mesh;
}

MeshData GeometryGenerator::CreatePrism(float width, float height, float depth, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float hw = width * 0.5f; float hh = height * 0.5f; float hd = depth * 0.5f;
    mesh.vertices = {
        { 0, hh, hd, 0,0,0, color.x, color.y, color.z, color.w }, { -hw, hh, -hd, 0,0,0, color.x, color.y, color.z, color.w }, { hw, hh, -hd, 0,0,0, color.x, color.y, color.z, color.w },
        { 0, -hh, hd, 0,0,0, color.x, color.y, color.z, color.w }, { -hw, -hh, -hd, 0,0,0, color.x, color.y, color.z, color.w }, { hw, -hh, -hd, 0,0,0, color.x, color.y, color.z, color.w }
    };
    mesh.indices = { 0,1,2,  3,5,4,  0,5,2,  0,3,5,  0,4,3,  0,1,4,  1,5,4,  1,2,5 };
    CalculateNormals(mesh);
    return mesh;
}

MeshData GeometryGenerator::CreateTetrahedron(float size, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float s = size; float h = s * 0.816f;
    mesh.vertices = {
        { 0.0f, h, 0.0f, 0,0,0, color.x, color.y, color.z, color.w },
        { 0.0f, 0.0f, -s * 0.577f, 0,0,0, color.x, color.y, color.z, color.w },
        { s * 0.5f, 0.0f, s * 0.289f, 0,0,0, color.x, color.y, color.z, color.w },
        { -s * 0.5f, 0.0f, s * 0.289f, 0,0,0, color.x, color.y, color.z, color.w }
    };
    mesh.indices = { 0, 1, 2,  0, 3, 1,  0, 2, 3,  1, 3, 2 };
    CalculateNormals(mesh);
    return mesh;
}

MeshData GeometryGenerator::CreateWedge(float width, float height, float depth, DirectX::XMFLOAT4 color) {
    MeshData mesh;
    float hw = width * 0.5f; float hh = height * 0.5f; float hd = depth * 0.5f;
    mesh.vertices = {
        { -hw, -hh, -hd, 0,0,0, color.x, color.y, color.z, color.w },
        {  hw, -hh, -hd, 0,0,0, color.x, color.y, color.z, color.w },
        { -hw, -hh,  hd, 0,0,0, color.x, color.y, color.z, color.w },
        {  hw, -hh,  hd, 0,0,0, color.x, color.y, color.z, color.w },
        { -hw,  hh,  hd, 0,0,0, color.x, color.y, color.z, color.w },
        {  hw,  hh,  hd, 0,0,0, color.x, color.y, color.z, color.w }
    };
    mesh.indices = { 0, 2, 1,  1, 2, 3,  2, 4, 3,  3, 4, 5,  0, 1, 5,  0, 5, 4,  0, 4, 2,  1, 3, 5 };
    CalculateNormals(mesh);
    return mesh;
}

MeshData GeometryGenerator::CreateCapsule(float radius, float height, int sliceCount, DirectX::XMFLOAT4 color) {
    return CreateSphere(radius, sliceCount, sliceCount, color);
}