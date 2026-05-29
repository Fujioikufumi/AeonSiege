#include "CombatHud.h"
#include "CombatHudSerializer.h"
#include "GameManager.h"
#include "Scene.h"
#include "Player.h"
#include "HealthComponent.h"
#include "FileUtil.h"
#include "NameSpace.h"
#include "PartyManager.h"
#include "PhaseManager.h"

bool CombatHud::Init() 
{
    if (!GameObject::Init()) return false;

    // 1. レイアウトデータの読み込み
    std::wstring jsonPath;
    if (SearchFilePathW(L"Assets/AppData/CombatHudLayout.json", jsonPath)) {
        CombatHudSerializer::Load(jsonPath, m_LayoutData);
    }
    else {
        // パスが空でもSerializer側でデフォルト値をApplyする
        CombatHudSerializer::Load(L"", m_LayoutData);
    }

    // 2. パネルの生成
    m_EnemyPanel = std::make_unique<EnemyHudPanel>(this, m_LayoutData);

    m_AllyPanels.clear();
    for (const auto& allyData : m_LayoutData.allies) {
        m_AllyPanels.push_back(std::make_unique<AllyHudPanel>(this, allyData));
    }

    m_SkillPanels.clear();
    for (int i = 0; i < 6; ++i) {
        std::wstring icon = L"Assets/Texture/Skill/Skill" + std::to_wstring(i + 1) + L".png";
        // 修正後:
        m_SkillPanels.push_back(std::make_unique<SkillHudPanel>(this, icon, i));
    }

	m_PhasePanel = std::make_unique<PhaseHudPanel>(this);

    RefreshLayout();
    return true;
}

void CombatHud::Update(float deltaTime) 
{
    Scene* scene = GameManager::GetScene();
    if (!scene) return;

    Player* player = scene->GetGameObjectByName<Player>("Player");
    if (!player) return;

    // パーティの成長情報を管理する PartyManager を取得
    PartyManager* party = scene->GetGameObjectByName<PartyManager>("PartyManager");
    const int currentLevel = party ? party->GetPartyLevel() : 1;
    const float expRatio   = party ? party->GetPartyExpRatio() : 0.0f;

    // 1. 味方パネルの更新
    for (auto& panel : m_AllyPanels) {
        GameObject* target = scene->GetGameObjectByName(panel->GetTargetName());
        if (target && !target->IsDestroyed()) {
            panel->SetVisible(true);
            auto* hp = target->GetComponent<HealthComponent>();
            if (hp) {
                // ここで実際のレベルと経験値比率を渡す
                panel->Update(hp->GetHP(), hp->GetMaxHP(), currentLevel, expRatio);
            }
        } else {
            panel->SetVisible(false);
        }
    }

    // 2. エネミーパネルの更新
    GameObject* lockTarget = const_cast<GameObject*>(player->GetLockTarget());
    if (lockTarget && !lockTarget->IsDestroyed()) {
        auto* enemyHp = lockTarget->GetComponent<HealthComponent>();
        if (enemyHp && enemyHp->IsAlive()) {
            m_EnemyPanel->SetVisible(true);
            float ratio = static_cast<float>(enemyHp->GetHP()) / max(1, enemyHp->GetMaxHP());
            m_EnemyPanel->Update(ratio, deltaTime);
        }
        else {
            m_EnemyPanel->SetVisible(false);
        }
    }
    else {
        m_EnemyPanel->SetVisible(false);
    }

    // スキルコンポーネントを取得
    auto* skillComp = player->GetComponent<SkillComponent>();


    // 3. スキルパネルの更新
    for (size_t i = 0; i < m_SkillPanels.size(); ++i) {
        bool hasSkill = false;
        float cd = 0.0f;

        if (skillComp) {
            // スロット i にスキルが設定されているか確認
            auto* skill = skillComp->GetSkillBySlot(static_cast<int>(i));
            if (skill != nullptr) {
                hasSkill = true;
                // クールダウン比率を取得
                cd = player->GetSkillCooldownRatioBySlot(static_cast<int>(i));
            }
        }

        // 有効フラグ(hasSkill)を渡して更新
        m_SkillPanels[i]->Update(cd, hasSkill);
    }

    PhaseManager* phaseManager = scene->GetGameObjectByName<PhaseManager>("PhaseManager");
    if (phaseManager && m_PhasePanel)
    {
        m_PhasePanel->Update(phaseManager->GetCurrentPhaseNumber());
    }

    // 各パーツが追加したコンポーネントを基底クラスで描画させるために呼ぶ
    GameObject::Update(deltaTime);
}

void CombatHud::RefreshLayout() 
{
    // 1. エネミーパネルの配置を更新
    if (m_EnemyPanel) {
        m_EnemyPanel->ApplyLayout();
    }

    // 2. 味方パネルの配置を更新
    for (size_t i = 0; i < m_AllyPanels.size(); ++i) {
        m_AllyPanels[i]->ApplyLayout(m_LayoutData.allies[i]);
    }

    // 3. スキルパネルの配置を更新
    const float iconPx  = HudLayoutUtil::ScreenHeightRatio(m_LayoutData.skillH);
    const float gap     = HudLayoutUtil::ScreenWidthRatio(m_LayoutData.skillGap);
    const float centerX = HudLayoutUtil::ScreenWidthRatio(m_LayoutData.skillCenterX);
    const float y       = HudLayoutUtil::ScreenHeightRatio(m_LayoutData.skillY);
    const int count = static_cast<int>(m_SkillPanels.size());

    for (int i = 0; i < count; ++i) {
        float offset = static_cast<float>(i) - (static_cast<float>(count) - 1.0f) * 0.5f;
        m_SkillPanels[i]->SetLayout(centerX + offset * gap, y, iconPx);
    }
}