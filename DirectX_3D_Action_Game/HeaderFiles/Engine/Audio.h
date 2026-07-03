#pragma once

#include <string>
#include <map>
#include <memory>
#include <Audio.h>

using namespace DirectX;

class Audio {
public:
	Audio();
	~Audio();

	bool Initialize();
	void Update();
	bool LoadSound(const std::string& key, const std::wstring& filname);
	void Play(const std::string& key, bool loop = false, float volume = 1.0f);
	void StopAll();
	void Stop(const std::string& key);

private:
	std::unique_ptr<AudioEngine> audioEngine;
	std::map<std::string, std::unique_ptr<SoundEffect>> soundEffects;
	std::map<std::string, std::unique_ptr<DirectX::SoundEffectInstance>> loopInstances;
};
