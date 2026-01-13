/*===================================================================
// ファイル: Audio.cpp
// 概要: Audioクラスの実装 (DirectXTK版)
=====================================================================*/
#include "Engine/Audio.h"
#include <Windows.h>

Audio::Audio(){}
Audio::~Audio(){
	if (audioEngine) {
		audioEngine->Suspend();
	}
}

bool Audio::Initialize() {
	//
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr)) {
		//
		if (hr != RPC_E_CHANGED_MODE)return false;
	}

	// オーディオエンジン作成
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
		soundEffects[key]->Play(volume, 0.0f, 0.0f);//音量、ピッチ、パン
	}
	//ループ再生
	else {
		auto instance = soundEffects[key]->CreateInstance();
		instance->SetVolume(volume);
		instance->Play(true);
		loopInstances.push_back(std::move(instance));
	}
}

void Audio::StopAll() {
	//ループ再生中の音を停止
	for (auto& instance : loopInstances) {
		instance->Stop();
	}
	loopInstances.clear();
}