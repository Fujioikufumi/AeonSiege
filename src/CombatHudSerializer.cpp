#include "CombatHudSerializer.h"
#include "EncodingUtils.h"
#include "Logger.h"
#include <nlohmann/json.hpp>
#include <fstream>

// 参考: nlohmann::jsonの基本的な使い方
// https://qiita.com/yohm/items/0f389ba5c5de4e2df9cf

using Json = nlohmann::json;

namespace {

	// 共通の読み込み/書き込みヘルパー
    void ReadBox(const Json& j, const char* key, HudBox& b) {
        if (!j.contains(key)) return;
        const auto& obj = j[key];
        b.cx = obj.value("cx", b.cx);
        b.cy = obj.value("cy", b.cy);
        b.w = obj.value("w", b.w);
        b.h = obj.value("h", b.h);
    }

    void WriteBox(Json& j, const char* key, const HudBox& b) {
        j[key] = { {"cx", b.cx}, {"cy", b.cy}, {"w", b.w}, {"h", b.h} };
    }

	// RGB配列の読み込み
    void ReadRgb(const Json& j, const char* key, std::array<float, 3>& c) {
        if (!j.contains(key) || !j[key].is_array() || j[key].size() < 3) return;
        for (int i = 0; i < 3; ++i) c[i] = j[key][i].get<float>();
    }

	// RGB配列の書き込み
    void WriteRgb(Json& j, const char* key, const std::array<float, 3>& c) {
        j[key] = { c[0], c[1], c[2] };
    }

	// 数値レイアウトの読み込み
    void ReadNumber(const Json& j, const char* keyX, const char* keyY, const char* keyScale, HudNumberLayout& n) {
        n.onesDigitX = j.value(keyX, n.onesDigitX);
        n.onesDigitY = j.value(keyY, n.onesDigitY);
        n.scale = j.value(keyScale, n.scale);
    }

    // --- Ally個別の変換 ---
    AllyLayout JsonToAlly(const Json& o) {
        AllyLayout a;
        a.hpFrom = o.value("hpFrom", "");
        if (o.contains("panelPath")) {
            a.panelPath = EncodeUtf8ToWide(o["panelPath"].get<std::string>());
        }

        ReadBox(o, "panel", a.panel);

        // HPセクション
        ReadBox(o, "hpGray", a.hp.barBack);
        ReadBox(o, "hpCol", a.hp.bar);
        ReadRgb(o, "colGray", a.hp.colBack);
        ReadRgb(o, "colHp", a.hp.colBar);
        ReadNumber(o, "curHpOnesDigitX", "curHpOnesDigitY", "hpNumScale", a.hp.number);

        // EXPセクション
        ReadBox(o, "expGray", a.exp.barBack);
        ReadBox(o, "expCol", a.exp.bar);
        ReadRgb(o, "expColBack", a.exp.colBack);
        ReadRgb(o, "expColBar", a.exp.colBar);

        // レベル
        ReadNumber(o, "lvOnesDigitX", "lvOnesDigitY", "lvNumScale", a.levelNumber);

        return a;
    }

    Json AllyToJson(const AllyLayout& a) {
        Json o;
        o["hpFrom"] = a.hpFrom;
        o["panelPath"] = EncodeWideToUtf8(a.panelPath); // 変換が必要
        o["showHpNum"] = a.showHpNum;

        WriteBox(o, "panel", a.panel);

        // HP
        WriteBox(o, "hpGray", a.hp.barBack);
        WriteBox(o, "hpCol", a.hp.bar);
        WriteRgb(o, "colGray", a.hp.colBack);
        WriteRgb(o, "colHp", a.hp.colBar);
        o["curHpOnesDigitX"] = a.hp.number.onesDigitX;
        o["curHpOnesDigitY"] = a.hp.number.onesDigitY;
        o["hpNumScale"] = a.hp.number.scale;

        // EXP
        WriteBox(o, "expGray", a.exp.barBack);
        WriteBox(o, "expCol", a.exp.bar);
        WriteRgb(o, "expColBack", a.exp.colBack);
        WriteRgb(o, "expColBar", a.exp.colBar);

        // Level
        o["lvOnesDigitX"] = a.levelNumber.onesDigitX;
        o["lvOnesDigitY"] = a.levelNumber.onesDigitY;
        o["lvNumScale"] = a.levelNumber.scale;
        return o;
    }
}

bool CombatHudSerializer::Load(const std::wstring& path, CombatHudLayout& out) {
    // 1. まずデフォルト値をセット（これをベースにする）
    ApplyDefaults(out);

    std::ifstream file(path);
    if (!file.is_open()) return true; // ファイルがないならデフォルトで成功

    try {
        Json j;
        file >> j;

        // 2. 全体設定の上書き (ここで enemyDecor 等を確実に取得)
        ReadBox(j, "enemyDecor", out.enemyDecor);
        ReadBox(j, "enemyBar", out.enemyBar);
        ReadRgb(j, "colGray", out.colGray);
        ReadRgb(j, "colHp", out.colHp);
        ReadRgb(j, "colHpLag", out.colHpLag);

        out.skillCenterX = j.value("skillCenterX", out.skillCenterX);
        out.skillY = j.value("skillY", out.skillY);
        out.skillH = j.value("skillH", out.skillH);
        out.skillGap = j.value("skillGap", out.skillGap);

        // 3. 味方情報があれば上書き (以前のロジック)
        if (j.contains("allies") && j["allies"].is_array()) {
            out.allies.clear();
            for (const auto& aJson : j["allies"]) {
                out.allies.push_back(JsonToAlly(aJson));
            }
        }
        return true;
    }
    catch (const std::exception& e) {
        ELOG("CombatHudSerializer::Load Error: %s", e.what());
        return false;
    }
}

bool CombatHudSerializer::Save(const std::wstring& path, const CombatHudLayout& layout) {
    try {
        Json j;
        WriteBox(j, "enemyDecor", layout.enemyDecor);
        WriteBox(j, "enemyBar", layout.enemyBar);
        WriteRgb(j, "colGray", layout.colGray);
        WriteRgb(j, "colHp", layout.colHp);
        WriteRgb(j, "colHpLag", layout.colHpLag);

        j["skillCenterX"] = layout.skillCenterX;
        j["skillY"] = layout.skillY;
        j["skillH"] = layout.skillH;
        j["skillGap"] = layout.skillGap;

        Json alliesJson = Json::array();
        for (const auto& ally : layout.allies) {
            alliesJson.push_back(AllyToJson(ally));
        }
        j["allies"] = alliesJson;

        // ファイル書き出し
        std::ofstream file(path);
        if (!file.is_open()) {
            ELOG("CombatHudSerializer::Save: Failed to open file for writing.");
            return false;
        }
        file << j.dump(4);
        return true;
    }
    catch (...) {
        return false;
    }
}

void CombatHudSerializer::ApplyDefaults(CombatHudLayout& out)
{
    // 1. 全体設定およびエネミーバー
    out.enemyDecor = { 0.50f, 0.085f, 0.40f, 0.06f };
    out.enemyBar = { 0.50f, 0.085f, 0.30f, 0.016f };
    out.colGray = { 0.45f, 0.45f, 0.48f };
    out.colHp = { 0.95f, 0.15f, 0.12f };
    out.colHpLag = { 0.75f, 0.12f, 0.10f };

    out.skillCenterX = 0.5f;
    out.skillY = 0.86f;
    out.skillH = 0.075f;
    out.skillGap = 0.065f;

    // 2. 味方の初期設定 (Player と Paladin)
    out.allies.clear();

    // Player
    AllyLayout a0;
    a0.hpFrom = "Player";
    a0.panelPath = L"Assets/Texture/CombatHud/Sigrun_CombatHud.png";
    a0.showHpNum = true;
    a0.panel = { 0.15f, 0.88f, 0.15f, 0.064f };
    a0.hp.barBack = { 0.30f, 0.775f, 0.22f, 0.014f };
    a0.hp.bar = a0.hp.barBack;
    a0.hp.number = { 0.30f, 0.775f - 0.038f, 0.22f };
    a0.exp.barBack = { 0.30f, 0.792f, 0.22f, 0.010f };
    a0.exp.bar = a0.exp.barBack;
    a0.levelNumber = { 0.08f, 0.75f, 0.22f };
    out.allies.push_back(a0);

    // Paladin
    AllyLayout a1;
    a1.hpFrom = "Paladin";
    a1.panelPath = L"Assets/Texture/CombatHud/Oscar_CombatHud.png";
    a1.showHpNum = true;
    a1.panel = { 0.15f, 0.79f, 0.15f, 0.064f };
    a1.hp.barBack = { 0.30f, 0.775f, 0.22f, 0.014f };
    a1.hp.bar = a1.hp.barBack;
    a1.hp.number = { 0.60f, 0.775f - 0.038f, 0.22f };
    a1.exp.barBack = { 0.30f, 0.792f, 0.22f, 0.010f };
    a1.exp.bar = a1.exp.barBack;
    a1.levelNumber = { 0.08f, 0.75f, 0.22f };
    out.allies.push_back(a1);
}
