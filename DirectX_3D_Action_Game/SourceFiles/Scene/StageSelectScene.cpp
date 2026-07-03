#include "Scene/StageSelectScene.h"
#include "Scene/GameScene.h"
#include "Scene/TitleScene.h"
#include "App/Main.h"
#include "Engine/Input.h"

StageSelectScene::StageSelectScene(SceneManager* manager) : BaseScene(manager) {}

static float ss_blinkTimer = 0.0f;

void StageSelectScene::Initialize() {
    pSkyBox = std::make_unique<SkyBox>();
    pSkyBox->Initialize(Game::GetInstance()->GetGraphics());

    m_selectIndex = Game::GetInstance()->GetMaxUnlockedStage() - 1;
    AppLog::AddLog("--- Stage Select ---");
    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Play("BGM_SELECT", true, 0.4f);
    }
}

void StageSelectScene::Update(float dt) {
    ss_blinkTimer += dt;
    Input* input = Game::GetInstance()->GetInput();
    int maxUnlock = Game::GetInstance()->GetMaxUnlockedStage();
    bool decided = false;

    DirectX::XMFLOAT2 mousePos = input->GetMousePosition();
    float mx = mousePos.x;
    float my = mousePos.y;

    static float prevMx = -1.0f;
    static float prevMy = -1.0f;
    bool isMouseMoved = (abs(mx - prevMx) > 1.0f || abs(my - prevMy) > 1.0f);
    prevMx = mx;
    prevMy = my;

    float startX = 150.0f;
    float startY = 200.0f;
    float btnW = 600.0f;
    float btnH = 60.0f;
    float gapY = 80.0f;

    int hoverIndex = -1;
    bool isHover = false;

    for (int i = 0; i < 3; ++i) {
        if (i >= maxUnlock) continue;

        float itemY = startY + (i * gapY);
        if (mx >= startX && mx <= startX + btnW &&
            my >= itemY && my <= itemY + btnH)
        {
            hoverIndex = i;
            isHover = true;
            break;
        }
    }

    if (isHover) {
        if (isMouseMoved && m_selectIndex != hoverIndex) {
            m_selectIndex = hoverIndex;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsMouseKeyDown(0)) {
            m_selectIndex = hoverIndex;
            decided = true;
        }
    }

    if (!decided) {
        if (input->IsKeyDown('W') || input->IsKeyDown(VK_UP)) {
            m_selectIndex--;
            if (m_selectIndex < 0) m_selectIndex = maxUnlock - 1;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsKeyDown('S') || input->IsKeyDown(VK_DOWN)) {
            m_selectIndex++;
            if (m_selectIndex >= maxUnlock) m_selectIndex = 0;
            if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_SWITCH");
        }
        if (input->IsKeyDown(VK_RETURN) || input->IsKeyDown(VK_SPACE)) {
            decided = true;
        }
    }

    if (decided) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_JUMP");
        Game::GetInstance()->SetCurrentStage(m_selectIndex + 1);
        Game::GetInstance()->SetCurrentPhase(1);
        Game::GetInstance()->SetSavedHP(-1);
        Game::GetInstance()->GetSceneManager()->ChangeScene<GameScene>();
    }

    if (input->IsKeyDown(VK_BACK) || input->IsKeyDown(VK_ESCAPE)) {
        if (auto audio = Game::GetInstance()->GetAudio()) audio->Play("SE_CANCEL");
        Game::GetInstance()->GetSceneManager()->ChangeScene<TitleScene>();
    }
}

void StageSelectScene::Draw() {
    Graphics* g = Game::GetInstance()->GetGraphics();

    if (pSkyBox) {
        XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -5, 0), XMVectorSet(0, 0, 0, 0), XMVectorSet(0, 1, 0, 0));
        XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45), 1280.0f / 720.0f, 0.1f, 1000.0f);
        pSkyBox->Draw(g, view, proj);
    }

    g->BeginDraw2D();
    float scW = (float)Config::SCREEN_WIDTH;
    float scH = (float)Config::SCREEN_HEIGHT;

    for (float y = 0; y < scH; y += 4.0f) {
        g->DrawRect(0, y, scW, 1.0f, 0x22000000);
    }

    bool isGlitch = (rand() % 100 < 2);
    float offX = isGlitch ? (rand() % 4 - 2.0f) : 0.0f;
    uint32_t headCol = isGlitch ? 0xFFFFFFFF : 0xFF00FF00;

    g->DrawString(L"MISSION SELECT", 100.0f + offX, 50.0f, 48.0f, headCol);
    g->DrawRect(0, 110, scW, 2.0f, 0xFF00FF00);

    int maxUnlock = Game::GetInstance()->GetMaxUnlockedStage();

    float startX = 100.0f;
    float startY = 200.0f;
    float btnW = 600.0f;
    float btnH = 60.0f;
    float gapY = 80.0f;
    float slantX = 30.0f;

    for (int i = 0; i < 3; ++i) {
        bool isUnlocked = (i < maxUnlock);
        bool isSelected = (i == m_selectIndex);

        float x = startX + (i * slantX);
        float y = startY + (i * gapY);

        if (isUnlocked && isSelected) {
            x += 20.0f + sinf(ss_blinkTimer * 5.0f) * 5.0f;
        }

        uint32_t bgCol = 0xAA222222;
        uint32_t lineCol = 0xFF444444;
        uint32_t textCol = 0xFF888888;

        std::wstring text = L"STAGE " + std::to_wstring(i + 1);

        if (isUnlocked) {
            if (isSelected) {
                bgCol = 0xCC004400;
                lineCol = 0xFF00FF00;
                textCol = 0xFFFFFFFF;
            }
            else {
                bgCol = 0x88002200;
                lineCol = 0xFF006600;
                textCol = 0xFFAAAAAA;
            }
            if (i == 2) text += L" : FINAL OPERATION";
            else text += L" : PATROL MISSION";
        }
        else {
            text += L" : [ LOCKED ]";
        }

        g->FillRect(x, y, btnW, btnH, bgCol);
        g->DrawRectOutline(x, y, btnW, btnH, 2.0f, lineCol);
        if (isUnlocked) g->FillRect(x - 5.0f, y, 5.0f, btnH, lineCol);

        g->DrawString(text.c_str(), x + 30.0f, y + 10.0f, 32.0f, textCol);
    }

    g->DrawString(L"[W/S]: Select   [ENTER]: Deploy", 150.0f, 650.0f, 24.0f, 0xFFFFFFFF);

    g->EndDraw2D();
}
void StageSelectScene::Shutdown() {
    pSkyBox.reset();
    if (auto audio = Game::GetInstance()->GetAudio()) {
        audio->Stop("BGM_SELECT");
    }
}