#pragma once
#include <Windows.h>
#include <memory>
#include "Engine/Graphics.h"
#include "Engine/Input.h"
#include "Engine/Audio.h"
#include "Scene/SceneManager.h"
#include "ECS/Components/PlayerComponent.h"

struct CustomizeData {
    int headID = 0;
    int bodyID = 0;
    int waistID = 0;
    int armLeftID = 0;
    int armRightID = 0;
    int legID = 0;
    int weaponRightID = 0;
    int weaponLeftID = 0;
};

class Game {
public:
    Game();
    ~Game();

    // ‰Šú‰»
    bool Initialize(HWND hWnd);
    // XV
    void Update(float dt);
    // •`‰æ
    void Draw();
    // I—¹ˆ—
    void Shutdown();

    static Game* GetInstance() { return instance; }

    Graphics* GetGraphics() const { return pGraphics.get(); }
    Input* GetInput() const { return pInput.get(); }
    Audio* GetAudio() const { return pAudio.get(); }
    SceneManager* GetSceneManager() const { return pSceneManager.get(); }
    HWND GetWindowHandle() const { return m_hWnd; }

    void SetPlayerType(PlayerType type) { m_selectedPlayerType = type; }
    PlayerType GetPlayerType() const { return m_selectedPlayerType; }

    void SetCustomizeData(const CustomizeData& data) { m_customizeData = data; }
    CustomizeData GetCustomizeData() const { return m_customizeData; }

    void SetCurrentStage(int stage) { m_currentStage = stage; }
    int GetCurrentStage() const { return m_currentStage; }
   
    void SetCurrentPhase(int phase) { m_currentPhase = phase; }
    int GetCurrentPhase() const { return m_currentPhase; }
    void NextPhase() { m_currentPhase++; }

    int GetMaxUnlockedStage() const { return m_maxUnlockedStage; }
    void UnlockNextStage() {
        if (m_currentStage >= m_maxUnlockedStage && m_maxUnlockedStage < 5) {
            m_maxUnlockedStage++;
        }
    }
    void SetSavedHP(int hp) { m_savedPlayerHP = hp; }
    int GetSavedHP() const { return m_savedPlayerHP; }

    void SaveGame();
    bool LoadGame(); 
    void ResetGame();

private:
    static Game* instance; 
    HWND m_hWnd = nullptr; 

    std::unique_ptr<Graphics> pGraphics;
    std::unique_ptr<Input> pInput;
    std::unique_ptr<Audio> pAudio;
    std::unique_ptr<SceneManager> pSceneManager;

    PlayerType m_selectedPlayerType = PlayerType::AssaultStriker;

    CustomizeData m_customizeData;

    int m_currentStage = 1;
    int m_currentPhase = 1;
    int m_maxUnlockedStage = 1;
    int m_savedPlayerHP = -1;
    int playerType = 0;
};