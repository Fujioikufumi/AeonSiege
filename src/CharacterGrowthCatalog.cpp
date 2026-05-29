#include "CharacterGrowthCatalog.h"
#include "GameObject.h"
#include "StatusComponent.h"
#include "HealthComponent.h"
#include "EnemyAIComponent.h"
#include "Logger.h"
#include "FileUtil.h"
#include "EncodingUtils.h"
#include "StatusData.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <algorithm>

namespace {

	// 敵の成長パターン。JSON では base と perLevel の組み合わせで表現する。
	struct EnemyGrowthProfile
	{
		StatusData base{};
		std::vector<nlohmann::json> perLevel{};
	};

	std::unordered_map<std::string, EnemyGrowthProfile> g_enemyGrowth;

	// JSON の base 部分を StatusData に読み込む。存在しないフィールドは変更しない。
	void ReadAbsoluteBaseFields(const nlohmann::json& j, StatusData& s)
	{
		if (j.contains("maxHp"))
			s.maxHp = j["maxHp"].get<int>();
		if (j.contains("attackPower"))
			s.attackPower = j["attackPower"].get<int>();
		if (j.contains("defensePower"))
			s.defensePower = j["defensePower"].get<int>();
		if (j.contains("moveSpeed"))
			s.moveSpeed = j["moveSpeed"].get<float>();
		if (j.contains("autoAttackInterval"))
			s.autoAttackInterval = j["autoAttackInterval"].get<float>();
		if (j.contains("criticalRate"))
			s.criticalRate = j["criticalRate"].get<float>();
		if (j.contains("criticalDamageRate"))
			s.criticalDamageRate = j["criticalDamageRate"].get<float>();
		if (j.contains("evasionRate"))
			s.evasionRate = j["evasionRate"].get<float>();
		if (j.contains("skillChainDamageRate"))
			s.skillChainDamageRate = j["skillChainDamageRate"].get<float>();
		if (j.contains("skillRangeRate"))
			s.skillRangeRate = j["skillRangeRate"].get<float>();
		if (j.contains("skillCooldownRate"))
			s.skillCooldownRate = j["skillCooldownRate"].get<float>();
		if (j.contains("skillPowerRate"))
			s.skillPowerRate = j["skillPowerRate"].get<float>();
		if (j.contains("skillDurationRate"))
			s.skillDurationRate = j["skillDurationRate"].get<float>();
		if (j.contains("attackRate"))
			s.attackRate = j["attackRate"].get<float>();
		if (j.contains("maxHpRate"))
			s.maxHpRate = j["maxHpRate"].get<float>();
		if (j.contains("moveSpeedRate"))
			s.moveSpeedRate = j["moveSpeedRate"].get<float>();
		if (j.contains("damageTakenRate"))
			s.damageTakenRate = j["damageTakenRate"].get<float>();
	}

	// JSON の perLevel 部分を StatusData に加算する。存在しないフィールドは変更しない。
	void AddDeltaFields(const nlohmann::json& j, StatusData& s)
	{
		if (j.contains("maxHp"))
			s.maxHp += j["maxHp"].get<int>();
		if (j.contains("attackPower"))
			s.attackPower += j["attackPower"].get<int>();
		if (j.contains("defensePower"))
			s.defensePower += j["defensePower"].get<int>();
		if (j.contains("moveSpeed"))
			s.moveSpeed += j["moveSpeed"].get<float>();
		if (j.contains("autoAttackInterval"))
			s.autoAttackInterval += j["autoAttackInterval"].get<float>();
		if (j.contains("criticalRate"))
			s.criticalRate += j["criticalRate"].get<float>();
		if (j.contains("criticalDamageRate"))
			s.criticalDamageRate += j["criticalDamageRate"].get<float>();
		if (j.contains("evasionRate"))
			s.evasionRate += j["evasionRate"].get<float>();
		if (j.contains("skillChainDamageRate"))
			s.skillChainDamageRate += j["skillChainDamageRate"].get<float>();
		if (j.contains("skillRangeRate"))
			s.skillRangeRate += j["skillRangeRate"].get<float>();
		if (j.contains("skillCooldownRate"))
			s.skillCooldownRate += j["skillCooldownRate"].get<float>();
		if (j.contains("skillPowerRate"))
			s.skillPowerRate += j["skillPowerRate"].get<float>();
		if (j.contains("skillDurationRate"))
			s.skillDurationRate += j["skillDurationRate"].get<float>();
		if (j.contains("attackRate"))
			s.attackRate += j["attackRate"].get<float>();
		if (j.contains("maxHpRate"))
			s.maxHpRate += j["maxHpRate"].get<float>();
		if (j.contains("moveSpeedRate"))
			s.moveSpeedRate += j["moveSpeedRate"].get<float>();
		if (j.contains("damageTakenRate"))
			s.damageTakenRate += j["damageTakenRate"].get<float>();
	}

	// 方式A: Lv1 = base のみ。Lv が 1 上がるたび perLevel の 1 段を加算。足りない分は末尾行を繰り返す
	StatusData BuildStatusAtLevel(const EnemyGrowthProfile& p, int level)
	{
		StatusData result = p.base;
		if (level <= 1 || p.perLevel.empty())
			return result;
		const int steps = level - 1;
		for (int i = 0; i < steps; ++i)
		{
			const size_t ix = std::min<size_t>(static_cast<size_t>(i), p.perLevel.size() - 1);
			AddDeltaFields(p.perLevel[ix], result);
		}
		return result;
	}

	// EnemyType を文字列キーに変換
	const char* EnemyTypeKey(EnemyType t)
	{
		switch (t)
		{
		case EnemyType::Zombie:
			return "Zombie";
		case EnemyType::SkeletonZombie:
			return "SkeletonZombie";
		case EnemyType::Mutant:
			return "Mutant";
		default:
			return "Zombie";
		}
	}

} // namespace

namespace CharacterGrowth {
	// 敵の成長テーブルを JSON から読み込む
	bool TryLoadEnemyGrowthTable(const std::wstring& path)
	{
		g_enemyGrowth.clear();
		std::wstring fullPath;
		if (!SearchFilePathW(path.c_str(), fullPath))
		{
			ELOG("CharacterGrowth: file not found: %s",
				EncodeWideToUtf8(path).c_str());
			return false;
		}

		// JSONファイルの読み込み
		const std::string utf8 = EncodeWideToUtf8(fullPath);
		std::ifstream ifs(utf8);
		if (!ifs.is_open())
		{
			ELOG("CharacterGrowth: cannot open: %s", utf8.c_str());
			return false;
		}

		nlohmann::json root;
		try
		{
			ifs >> root;
		}
		catch (const std::exception& e)
		{
			ELOG("CharacterGrowth: JSON parse error: %s", e.what());
			return false;
		}

		if (!root.contains("enemies") || !root["enemies"].is_object())
		{
			ELOG("CharacterGrowth: missing \"enemies\" object.");
			return false;
		}

		for (const auto& kv : root["enemies"].items())
		{
			const nlohmann::json& slot = kv.value();
			if (!slot.is_object())
				continue;

			EnemyGrowthProfile prof;
			if (slot.contains("base") && slot["base"].is_object())
				ReadAbsoluteBaseFields(slot["base"], prof.base);

			if (slot.contains("perLevel") && slot["perLevel"].is_array())
			{
				for (const auto& row : slot["perLevel"])
				{
					if (row.is_object())
						prof.perLevel.push_back(row);
				}
			}
			g_enemyGrowth[kv.key()] = std::move(prof);
		}

		if (g_enemyGrowth.empty())
		{
			ELOG("CharacterGrowth: no enemy profiles loaded.");
			return false;
		}
		return true;
	}

	// 敵のスポーン時にステータスを適用する
	bool ApplyEnemySpawnStats(GameObject* enemy, EnemyType type, int level)
	{
		if (enemy == nullptr)
			return false;

		level = max(1, level);

		if (EnemyAIComponent* ai = enemy->GetComponent<EnemyAIComponent>())
			ai->SetLevel(level);

		const auto it = g_enemyGrowth.find(EnemyTypeKey(type));
		if (it == g_enemyGrowth.end())
			return false;

		StatusComponent* st = enemy->GetComponent<StatusComponent>();
		HealthComponent* hp = enemy->GetComponent<HealthComponent>();
		if (st == nullptr || hp == nullptr)
			return false;

		const StatusData built = BuildStatusAtLevel(it->second, level);
		st->Setup(built);

		const int maxHp = st->GetMaxHp();
		hp->SetMaxHP(maxHp);
		hp->SetHP(maxHp);
		return true;
	}

} // namespace CharacterGrowth