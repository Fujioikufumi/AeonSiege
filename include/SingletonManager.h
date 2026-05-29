#pragma once

//-----------------------------------------------------------------------------
//		シングルトンマネージャークラス
//-----------------------------------------------------------------------------
template <typename T>
class SingletonManager
{
protected:
	SingletonManager() = default;
	virtual ~SingletonManager() = default;

	// コピー・ムーブ禁止
	SingletonManager(const SingletonManager&) = delete;
	SingletonManager& operator=(const SingletonManager&) = delete;
	SingletonManager(SingletonManager&&) = delete;
	SingletonManager& operator=(SingletonManager&&) = delete;

public:
	/// <summary>
	/// インスタンス取得
	/// </summary>
	static T& GetInstance()
	{
		static T instance;
		return instance;
	}
};