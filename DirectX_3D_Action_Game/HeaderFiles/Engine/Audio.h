/*===================================================================
// ファイル: Audio.h
// 概要: DirectXTKを使用したオーディオ管理クラス
=====================================================================*/
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

	//初期化
	bool Initialize();

	//更新処理
	void Update();
	//音声読み込み
	//key:呼び出し用ID
	//filename:ファイルパス
	bool LoadSound(const std::string& key, const std::wstring& filname);

	//再生
	//loop：ループ再生するかどうか
	void Play(const std::string& key, bool loop = false, float volume = 1.0f);

	//停止
	void StopAll();

private:
	// オーディオエンジン
	std::unique_ptr<AudioEngine> audioEngine;

	//読み込んだ効果音データ
	//SoundEffect：音のデータ本体
	std::map<std::string, std::unique_ptr<SoundEffect>> soundEffects;

	//再生中のインスタンス管理
	//SoundEffectInstance：再生中の「音」の制御用
	std::vector<std::unique_ptr<SoundEffectInstance>> loopInstances;
};
