#include "Engine/Audio.h"
#include <Windows.h>

Audio::Audio(){}
Audio::~Audio(){
	if (audioEngine) {
		audioEngine->Suspend();
	}
}

bool Audio::Initialize() {
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		if (hr != RPC_E_CHANGED_MODE)return false;
	}

	AUDIO_ENGINE_FLAGS eflags = AudioEngine_Default;
#ifdef _DEBUG
	eflags |= AudioEngine_Debug;
#endif
	try {
		audioEngine = std::make_unique<AudioEngine>(eflags);
	}
	catch (...) {
		return false;
	}
	return true;
}

void Audio::Update() {
	if (!audioEngine) return;
	if (!audioEngine->Update()) {
		if (audioEngine->IsCriticalError()) {
		}
	}
}

bool Audio::LoadSound(const std::string& key, const std::wstring& filename) {
	if (!audioEngine) return false;

	try {
		auto se = std::make_unique<SoundEffect>(audioEngine.get(), filename.c_str());
		soundEffects[key] = std::move(se);
		return true;
	}
	catch (...) {
		std::wstring msg = L"Failed to load sound: " + filename;
		MessageBoxW(NULL, msg.c_str(), L"Audio Load Error", MB_OK);
		return false;
	}
}
void Audio::Play(const std::string& key, bool loop, float volume) {
	if (soundEffects.find(key) == soundEffects.end()) return;

	//ワンショット再生
	if (!loop) {
		soundEffects[key]->Play(volume, 0.0f, 0.0f);
	}
	//ループ再生
	else {
		if (loopInstances.find(key) != loopInstances.end()) {
			loopInstances[key]->Stop();
		}

		auto instance = soundEffects[key]->CreateInstance();
		instance->SetVolume(volume);
		instance->Play(true);

		loopInstances[key] = std::move(instance);
	}
}

void Audio::Stop(const std::string& key) {
	auto it = loopInstances.find(key);
	if (it != loopInstances.end()) {
		it->second->Stop();
		loopInstances.erase(it);
	}
}

void Audio::StopAll() {
	for (auto& pair : loopInstances) {
		if (pair.second) {
			pair.second->Stop();
		}
	}
	loopInstances.clear();
}