#include "../HeaderFiles/Engine/AnimationManager.h"
#include <fstream>
#include "../External/json.hpp"


using json = nlohmann::json;
using namespace DirectX;

PartType AnimationManager::StringToPartType(const std::string& str) {
    if (str == "Head") return PartType::Head;
    if (str == "Body") return PartType::Body;
    if (str == "Waist") return PartType::Waist;
    if (str == "ShoulderLeft") return PartType::ShoulderLeft;
    if (str == "ShoulderRight") return PartType::ShoulderRight;
    if (str == "ArmLeft") return PartType::ArmLeft;
    if (str == "ArmRight") return PartType::ArmRight;
    if (str == "LegLeft") return PartType::LegLeft;
    if (str == "LegRight") return PartType::LegRight;
    return PartType::Body;
}

bool AnimationManager::LoadAnimation(const std::string& name, const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    json j;
    file >> j;

    AnimationClip clip;
    clip.name = name;
    clip.duration = j.value("duration", 1.0f);

    if (j.contains("tracks")) {
        for (const auto& trackJson : j["tracks"]) {
            AnimTrack track;
            track.part = StringToPartType(trackJson["part"].get<std::string>());

            for (const auto& kfJson : trackJson["keyframes"]) {
                Keyframe kf;
                kf.time = kfJson["time"].get<float>();
                auto rot = kfJson["rot"];
                kf.rotation = XMFLOAT3(rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>());
                track.keyframes.push_back(kf);
            }
            clip.tracks[track.part] = track;
        }
    }
    m_animations[name] = clip;
    return true;
}

float AnimationManager::GetDuration(const std::string& animName) {
    if (m_animations.count(animName)) return m_animations[animName].duration;
    return 1.0f;
}

XMVECTOR AnimationManager::EvaluateTrack(const std::string& animName, PartType part, float currentTime) {
    if (m_animations.count(animName) == 0) return XMVectorZero();
    const auto& clip = m_animations[animName];
    if (clip.tracks.count(part) == 0) return XMVectorZero();

    const auto& track = clip.tracks.at(part);
    if (track.keyframes.empty()) return XMVectorZero();

    if (currentTime <= track.keyframes.front().time) {
        return XMLoadFloat3(&track.keyframes.front().rotation);
    }
    if (currentTime >= track.keyframes.back().time) {
        return XMLoadFloat3(&track.keyframes.back().rotation);
    }

    for (size_t i = 0; i < track.keyframes.size() - 1; ++i) {
        const auto& kf1 = track.keyframes[i];
        const auto& kf2 = track.keyframes[i + 1];

        if (currentTime >= kf1.time && currentTime <= kf2.time) {
            float t = (currentTime - kf1.time) / (kf2.time - kf1.time);
            XMVECTOR v1 = XMLoadFloat3(&kf1.rotation);
            XMVECTOR v2 = XMLoadFloat3(&kf2.rotation);
            return XMVectorLerp(v1, v2, t);
        }
    }
    return XMVectorZero();
}