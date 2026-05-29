#pragma once
#include "StatusData.h"
#include <string>

namespace PartyGrowth
{
	bool TryLoadPartyGrowth(const std::wstring& path = L"Assets/AppData/PartyGrowth.json");

	bool BuildMemberStatus(const char* memberJsonKey, int partyLevel, StatusData& out);

	int GetExpNeededForNextLevel(int currentPartyLevel);

	int GetMaxPartyLevelFromTable();

	bool IsLoaded();
}