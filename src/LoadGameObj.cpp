#include "LoadGameObj.h"
#include "ModelManager.h"
#include "AnimationManager.h"
#include <vector>
#include <future>
void LoadGameObj();

void LoadGameObj()
{
    std::vector<std::future<void>> loadTasks;

    loadTasks.push_back(std::async(std::launch::async, []() {
        auto& animMgr = AnimationManager::GetInstance();
        animMgr.LoadAnimation(L"Assets/Animations/Walking.banm", "Walk");
        animMgr.LoadAnimation(L"Assets/Animations/Running.banm", "Run");
        animMgr.LoadAnimation(L"Assets/Animations/Jump.banm", "Jump");
        animMgr.LoadAnimation(L"Assets/Animations/Idle.banm", "Idle");
        }));
    loadTasks.push_back(std::async(std::launch::async, []() {
        auto& animMgr = AnimationManager::GetInstance();
        animMgr.LoadAnimation(L"Assets/Animations/Great_Sword_Slash.banm", "Player_Attack01");
        animMgr.LoadAnimation(L"Assets/Animations/Great_Sword_Slash1.banm", "Player_Skill01");
        animMgr.LoadAnimation(L"Assets/Animations/Great_Sword_Casting.banm", "Player_Skill02");
        animMgr.LoadAnimation(L"Assets/Animations/Great_Sword_Idle.banm", "Player_Combat_Idle");
        }));
    // タスク3: ミュータント用アニメーションの読み込み（別スレッド）
    loadTasks.push_back(std::async(std::launch::async, []() {
        auto& animMgr = AnimationManager::GetInstance();
        animMgr.LoadAnimation(L"Assets/Animations/Mutant_Walking.banm", "Mutant_Normal_Walk");
        animMgr.LoadAnimation(L"Assets/Animations/Mutant_Idle.banm", "Mutant_Normal_Idle");
        animMgr.LoadAnimation(L"Assets/Animations/Mutant_Run.banm", "Mutant_Combat_Run");
        animMgr.LoadAnimation(L"Assets/Animations/Mutant_Swiping.banm", "Mutant_Combat_Swiping");
        animMgr.LoadAnimation(L"Assets/Animations/Mutant_Dying.banm", "Mutant_Combat_Dying");
        }));
    for(auto& task : loadTasks)
    {
        task.get(); // ここで各スレッドのタスク完了を待つ
    }
}
