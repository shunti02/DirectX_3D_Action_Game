#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <DirectXMath.h>
#include "ECS/Components/PlayerPartComponent.h" 

struct Keyframe {
    float time;
    DirectX::XMFLOAT3 rotation;
};

struct AnimTrack {
    PartType part;
    std::vector<Keyframe> keyframes;
};

struct AnimationClip {
    std::string name;
    float duration;
    std::map<PartType, AnimTrack> tracks;
};

class AnimationManager {
public:
    static AnimationManager* GetInstance() {
        static AnimationManager instance;
        return &instance;
    }

    bool LoadAnimation(const std::string& name, const std::string& filepath);
    DirectX::XMVECTOR EvaluateTrack(const std::string& animName, PartType part, float currentTime);
    float GetDuration(const std::string& animName);

private:
    AnimationManager() {}
    ~AnimationManager() {}

    std::unordered_map<std::string, AnimationClip> m_animations;
    PartType StringToPartType(const std::string& str);
};