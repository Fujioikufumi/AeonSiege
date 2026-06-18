#pragma once
//----------------------------------------------------------------------
//		Includes
//----------------------------------------------------------------------
#include <d3d12.h>
#include <d3dcompiler.h>
#include <ComPtr.h>
#include <string>
#include <map>
#include "SingletonManager.h"

//----------------------------------------------------------------------
//		Linker
//----------------------------------------------------------------------
#pragma comment(lib, "d3dcompiler.lib")

struct ShaderInfo
{
	ComPtr<ID3DBlob> pVSBlob  = nullptr;	// 頂点シェーダー
	ComPtr<ID3DBlob> pPSBlob  = nullptr;	// ピクセルシェーダー
	ComPtr<ID3DBlob> pGSBlob = nullptr;		// ジオメトリシェーダー
	ComPtr<ID3DBlob> pCSBlob = nullptr;		// コンピュートシェーダー
	std::wstring vsPath = {};				// 頂点シェーダーのパス
	std::wstring psPath = {};				// ピクセルシェーダ―のパス
	std::wstring gsPath = {};				// ジオメトリシェーダーのパス
	std::wstring csPath = {};				// コンピュートシェーダーのパス
	bool isValid = false;					// 有効かどうか
};

//----------------------------------------------------------------------
//		シェーダーマネージャークラス
//----------------------------------------------------------------------
class ShaderManager : public SingletonManager<ShaderManager>
{
private:
	friend class SingletonManager<ShaderManager>;

	std::map<std::wstring, ShaderInfo> m_LoadedShaders;

	ShaderManager() = default;
	~ShaderManager() = default;
	ShaderManager(const ShaderManager&) = delete;
	void operator=(const ShaderManager&) = delete;

	/// <summary>
	/// シェーダーファイルを読み込む
	/// </summary>
	bool LoadShaderFile(const std::wstring& shaderPath, ComPtr<ID3DBlob>& pBlob);

public:

	/// <summary>
	// シェーダーを読み込む
	/// </summary>
	bool LoadShader(
		const std::wstring& shaderName,
		const std::wstring& vsPath,
		const std::wstring& psPath);

	/// <summary>
	/// ジオメトリシェーダーも含めてシェーダーを読み込む
	/// </summary>
	bool LoadShader(
		const std::wstring& shaderName,
		const std::wstring& vsPath,
		const std::wstring& psPath,
		const std::wstring& gsPath);

	/// <summary>
	/// コンピュートシェーダーを読み込む
	/// </summary>
	bool LoadComputeShader(
		const std::wstring& shaderName, 
		const std::wstring& csPath);

	/// <summary>
	/// シェーダー情報を取得
	/// </summary>
	ShaderInfo* GetShader(const std::wstring& shaderName);

	/// <summary>
	/// コンピュートシェーダー情報を取得
	/// </summary>
	ShaderInfo* GetComputeShader(const std::wstring& shaderName);

	/// <summary>
	/// シェーダーが読み込まれているかどうか
	/// </summary>
	bool IsShaderLoaded(const std::wstring& shaderName) const;

	/// <summary>
	/// シェーダーをアンロード
	/// </summary>
	void UnloadShader(const std::wstring& shaderName);

	/// <summary>
	/// 全てのシェーダーをアンロード
	/// </summary>
	void UnloadAll();

	/// <summary>
	/// 読み込まれているシェーダーの数を取得
	/// </summary>
	size_t GetLoadedShaderCount() const { return m_LoadedShaders.size(); }

	/// <summary>
	/// 終了処理
	/// </summary>
	void Term();
};