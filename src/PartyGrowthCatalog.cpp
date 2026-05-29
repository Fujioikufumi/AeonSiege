#include "PartyGrowthCatalog.h"
#include "Logger.h"
#include "FileUtil.h"
#include "EncodingUtils.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace {

	struct MemberProfile
	{
		StatusData base{};
		std::vector<nlohmann::json> perLevel{};
	};

	std::vector<int> g_expToNext;
	std::unordered_map<std::string, MemberProfile> g_members;
	bool g_loaded = false;

	void ReadAbsoluteBase(const nlohmann::json& j, StatusData& s)
	{
		if (j.contains("maxHp")) s.maxHp = j["maxHp"].get<int>();
		if (j.contains("attackPower")) s.attackPower = j["attackPower"].get<int>();
		if (j.contains("defensePower")) s.defensePower = j["defensePower"].get<int>();
		if (j.contains("moveSpeed")) s.moveSpeed = j["moveSpeed"].get<float>();
		if (j.contains("autoAttackInterval")) s.autoAttackInterval = j["autoAttackInterval"].get<float>();
		if (j.contains("criticalRate")) s.criticalRate = j["criticalRate"].get<float>();
		if (j.contains("criticalDamageRate")) s.criticalDamageRate = j["criticalDamageRate"].get<float>();
		if (j.contains("evasionRate")) s.evasionRate = j["evasionRate"].get<float>();
		if (j.contains("skillChainDamageRate")) s.skillChainDamageRate = j["skillChainDamageRate"].get<float>();
		if (j.contains("skillRangeRate")) s.skillRangeRate = j["skillRangeRate"].get<float>();
		if (j.contains("skillCooldownRate")) s.skillCooldownRate = j["skillCooldownRate"].get<float>();
		if (j.contains("skillPowerRate")) s.skillPowerRate = j["skillPowerRate"].get<float>();
		if (j.contains("skillDurationRate")) s.skillDurationRate = j["skillDurationRate"].get<float>();
		if (j.contains("attackRate")) s.attackRate = j["attackRate"].get<float>();
		if (j.contains("maxHpRate")) s.maxHpRate = j["maxHpRate"].get<float>();
		if (j.contains("moveSpeedRate")) s.moveSpeedRate = j["moveSpeedRate"].get<float>();
		if (j.contains("damageTakenRate")) s.damageTakenRate = j["damageTakenRate"].get<float>();
	}

	void AddDelta(const nlohmann::json& j, StatusData& s)
	{
		if (j.contains("maxHp")) s.maxHp += j["maxHp"].get<int>();
		if (j.contains("attackPower")) s.attackPower += j["attackPower"].get<int>();
		if (j.contains("defensePower")) s.defensePower += j["defensePower"].get<int>();
		if (j.contains("moveSpeed")) s.moveSpeed += j["moveSpeed"].get<float>();
		if (j.contains("autoAttackInterval")) s.autoAttackInterval += j["autoAttackInterval"].get<float>();
		if (j.contains("criticalRate")) s.criticalRate += j["criticalRate"].get<float>();
		if (j.contains("criticalDamageRate")) s.criticalDamageRate += j["criticalDamageRate"].get<float>();
		if (j.contains("evasionRate")) s.evasionRate += j["evasionRate"].get<float>();
		if (j.contains("skillChainDamageRate")) s.skillChainDamageRate += j["skillChainDamageRate"].get<float>();
		if (j.contains("skillRangeRate")) s.skillRangeRate += j["skillRangeRate"].get<float>();
		if (j.contains("skillCooldownRate")) s.skillCooldownRate += j["skillCooldownRate"].get<float>();
		if (j.contains("skillPowerRate")) s.skillPowerRate += j["skillPowerRate"].get<float>();
		if (j.contains("skillDurationRate")) s.skillDurationRate += j["skillDurationRate"].get<float>();
		if (j.contains("attackRate")) s.attackRate += j["attackRate"].get<float>();
		if (j.contains("maxHpRate")) s.maxHpRate += j["maxHpRate"].get<float>();
		if (j.contains("moveSpeedRate")) s.moveSpeedRate += j["moveSpeedRate"].get<float>();
		if (j.contains("damageTakenRate")) s.damageTakenRate += j["damageTakenRate"].get<float>();
	}

	StatusData BuildAtLevel(const MemberProfile& p, int level)
	{
		StatusData result = p.base;
		if (level <= 1 || p.perLevel.empty())
			return result;
		const int steps = level - 1;
		for (int i = 0; i < steps; ++i)
		{
			const size_t ix = std::min<size_t>(static_cast<size_t>(i), p.perLevel.size() - 1);
			AddDelta(p.perLevel[ix], result);
		}
		return result;
	}

} // namespace

namespace PartyGrowth {

	bool IsLoaded()
	{
		return g_loaded;
	}

	bool TryLoadPartyGrowth(const std::wstring& path)
	{
		g_loaded = false;
		g_expToNext.clear();
		g_members.clear();

		std::wstring fullPath;
		if (!SearchFilePathW(path.c_str(), fullPath))
		{
			ELOG("PartyGrowth: file not found: %s", EncodeWideToUtf8(path).c_str());
			return false;
		}
		std::ifstream ifs(EncodeWideToUtf8(fullPath));
		if (!ifs.is_open())
		{
			ELOG("PartyGrowth: cannot open");
			return false;
		}
		nlohmann::json root;
		try {
			ifs >> root;
		}
		catch (const std::exception& e) {
			ELOG("PartyGrowth JSON: %s", e.what());
			return false;
		}

		if (root.contains("party") && root["party"].contains("expToNext"))
		{
			for (const auto& v : root["party"]["expToNext"])
			{
				if (v.is_number_integer())
					g_expToNext.push_back(max(1, v.get<int>()));
			}
		}

		if (root.contains("members") && root["members"].is_object())
		{
			for (const auto& kv : root["members"].items())
			{
				const auto& slot = kv.value();
				if (!slot.is_object()) continue;
				MemberProfile prof;
				if (slot.contains("base") && slot["base"].is_object())
					ReadAbsoluteBase(slot["base"], prof.base);
				if (slot.contains("perLevel") && slot["perLevel"].is_array())
				{
					for (const auto& row : slot["perLevel"])
						if (row.is_object()) prof.perLevel.push_back(row);
				}
				g_members[kv.key()] = std::move(prof);
			}
		}

		g_loaded = !g_members.empty();
		return g_loaded;
	}

	bool BuildMemberStatus(const char* memberJsonKey, int partyLevel, StatusData& out)
	{
		if (!g_loaded || memberJsonKey == nullptr)
			return false;
		partyLevel = max(1, partyLevel);
		auto it = g_members.find(memberJsonKey);
		if (it == g_members.end())
			return false;
		out = BuildAtLevel(it->second, partyLevel);
		return true;
	}

	int GetExpNeededForNextLevel(int currentPartyLevel)
	{
		if (currentPartyLevel < 1)
			currentPartyLevel = 1;
		const int idx = currentPartyLevel - 1;
		if (idx < 0 || static_cast<size_t>(idx) >= g_expToNext.size())
			return 0;
		return g_expToNext[static_cast<size_t>(idx)];
	}

	int GetMaxPartyLevelFromTable()
	{
		return static_cast<int>(g_expToNext.size()) + 1;
	}

} // namespace PartyGrowth