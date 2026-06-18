#pragma once
#include "main.h"

// 画面サイズ
const uint32_t SCREEN_WIDTH = 1280;//960;
const uint32_t SCREEN_HEIGHT = 720;

extern bool g_isDebugMode;


enum class eLayer
{
	BACKGROUND = 0,	// 背景（奥から描画）
	TERRAIN,		// 地形
	Environment,	// 環境
	EFFECT,			// エフェクト
	DEFAULT,		// デフォルト
	PLAYER,			// プレイヤー
	ALLY,			// 味方
	ENEMY,			// 敵
	BULLET,			// 弾
	PLAYER_BULLET,	// プレイヤーの弾
	ENEMY_BULLET,	// 敵の弾
	WALL,			// 壁など
	ITEM,			// アイテム
	UI,				// UI（深度なしの別パスで描画する想定）
	MENUUI,			// メニューUI
	MAX				// 種類数
};

enum POOL_TYPE
{
	POOL_TYPE_RES = 0,     // CBV / SRV / UAV
	POOL_TYPE_SMP = 1,     // Sampler
	POOL_TYPE_RTV = 2,     // RTV
	POOL_TYPE_DSV = 3,     // DSV
	POOL_COUNT = 4,
};

namespace {

	struct alignas(256) CbMesh
	{
		XMFLOAT4X4 World; // ワールド行列
	};

	struct alignas(256) CbTransform
	{
		XMFLOAT4X4 View; // ビュー行列
		XMFLOAT4X4 Proj; // プロジェクション行列
	};

	/// <summary>
	/// ライト定数バッファ構造体
	/// </summary>
	struct alignas(256) CbLight
	{
		XMFLOAT3 LightColor;
		float LightIntensity;
		XMFLOAT3 LightForward;
		float Padding1; // パディング
		XMFLOAT3 AmbientColor; // 環境光
		float AmbientIntensity;

	};

	/// <summary>
	/// カメラ定数バッファ構造体
	/// </summary>
	struct alignas(256) CbCamera
	{
		XMFLOAT3 cbCamPos;
	};

	/// <summary>
	/// マテリアル定数バッファ構造体
	/// </summary>
	struct alignas(256) CbMaterial
	{
		XMFLOAT3 BaseColor; // ベースカラー
		float Alpha;        // 透明度
		float Roughness;    
		float Metallic;
		uint32_t HasBaseColorMap = 0;
		uint32_t HasMetallicMap = 0;
		uint32_t HasRoughnessMap = 0;
		uint32_t HasNormalMap = 0;
	};

} // namespace