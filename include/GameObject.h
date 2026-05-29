#pragma once
//======================================================================
//		Includes
//======================================================================
#include "DirectXMath.h"
#include "Mesh.h"
#include "Material.h"
#include "RenderContext.h"
#include "VertexBuffer.h"
#include "ConstantBuffer.h"
#include "DescriptorPool.h"
#include "Logger.h"
#include "FileUtil.h"
#include "Component.h"
#include "DamageTypes.h"
#include <string>
#include <vector>
#include <memory>
#include <list>

#include <nlohmann/json.hpp>

using namespace DirectX;

/// <summary>
/// シーンに追加するオブジェクトの基底クラス。
/// すべてのゲーム内オブジェクトはこのクラスを継承します。
/// </summary>
class GameObject
{
public:
	GameObject();
	virtual ~GameObject();

	/// 初期化処理
	virtual bool Init();

	/// 終了処理
	virtual void Term();

	/// 更新処理：所有しているコンポーネントの更新を行います。
	virtual void Update(float deltaTime = 0.0f);

	/// 描画処理：所有しているコンポーネントの描画を行います。
	virtual void Draw(const RenderContext& context);

	/// オブジェクトの削除フラグを立てる
	void Destroy() { m_IsDestroyed = true; }

	/// ダメージ処理（キャラクター等の派生クラスでオーバーライド）
	virtual DamageResult ApplyDamage(const DamageContext& context);

	//--------------------------------------------------------------------
	// Setters

	void SetPosition(const XMFLOAT3& position) { m_Position = position; }
	void SetRotation(const XMFLOAT3& rotation) { m_Rotation = rotation; }
	void SetScale(const XMFLOAT3& scale) { m_Scale = scale; }
	void SetModelPath(const std::wstring& modelPath) { m_ModelPath = modelPath; }
	void SetObjName(const std::string& name) { m_Name = name; }
	void SetBoundingRadius(float radius) { m_BoundingRadius = radius; }
	void SetIsCulled(bool isCulled) { m_IsCulled = isCulled; }
	void SetPipelineName(const std::wstring& pipelineName) { m_PipelineName = pipelineName; }
	void SetLayer(eLayer layer) { m_Layer = layer; }

	//--------------------------------------------------------------------
	// Getters

	[[nodiscard]] XMFLOAT3 GetPosition() const { return m_Position; }
	[[nodiscard]] XMFLOAT3 GetRotation() const { return m_Rotation; }
	[[nodiscard]] XMFLOAT3 GetScale() const { return m_Scale; }
	[[nodiscard]] eLayer GetLayer() const { return m_Layer; }
	[[nodiscard]] bool IsDestroyed() const { return m_IsDestroyed; }
	[[nodiscard]] bool IsCulled() const { return m_IsCulled; }
	[[nodiscard]] float GetBoundingRadius() const { return m_BoundingRadius; }
	[[nodiscard]] std::string GetObjName() const { return m_Name; }
	[[nodiscard]] std::wstring GetModelPath() const { return m_ModelPath; }
	[[nodiscard]] std::list<std::unique_ptr<Component>>& GetComponents() { return m_Components; }

	/// ワールド行列を取得
	[[nodiscard]] virtual XMMATRIX GetWorldMatrix() const;

	/// オブジェクトが向いている方向を取得
	[[nodiscard]] virtual XMFLOAT3 GetForward() const;

protected:
	// 座標・回転・スケール
	XMFLOAT3 m_Position = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_Rotation = { 0.0f, 0.0f, 0.0f };
	XMFLOAT3 m_Scale = { 1.0f, 1.0f, 1.0f };
	XMMATRIX m_WorldMatrix = {};

	// オブジェクト設定
	std::string  m_Name = "";
	std::wstring m_ModelPath = L"";
	std::wstring m_PipelineName = L"";
	eLayer       m_Layer = eLayer::DEFAULT;

	// 状態フラグ
	bool m_IsDestroyed = false;
	bool m_IsCulled = false;

	// カリング・当たり判定用
	float m_BoundingRadius = 1.0f;

	// コンポーネントリスト
	std::list<std::unique_ptr<Component>> m_Components;

	// 描画に必要なワールド行列の更新
	void UpdateWorldMatrix();
public:
	/// <summary>
	/// 新しいコンポーネントを追加します。
	/// </summary>
	/// <typeparam name="T">追加するコンポーネントの型。</typeparam>
	/// <returns>追加されたコンポーネントへの生ポインタ。</returns>
	template <typename T = Component>
	T* AddComponent()
	{
		auto component = std::make_unique<T>(this);
		T* rawPtr = component.get();
		m_Components.push_back(std::move(component));
		rawPtr->Init();
		return rawPtr;
	}

	/// <summary>
	/// 指定された型のコンポーネントを取得します。
	/// </summary>
	/// <typeparam name="T">取得するコンポーネントの型。</typeparam>
	/// <returns>指定された型のコンポーネントへのポインタ。見つからない場合は nullptr を返します。</returns>
	template <typename T = Component>
	T* GetComponent()
	{
		for (const auto& component : m_Components)
		{
			// dynamic_castは多用するとパフォーマンスに影響が出る可能性がある
			if (T* retPtr = dynamic_cast<T*>(component.get()))
				return retPtr;
		}
		return nullptr;
	}
};