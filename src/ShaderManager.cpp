//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "ShaderManager.h"
#include "Logger.h"
#include "FileUtil.h"

//----------------------------------------------------------------------
/// シェーダファイルを読み込む
//----------------------------------------------------------------------
bool ShaderManager::LoadShaderFile(const std::wstring& shaderPath, ComPtr<ID3DBlob>& pBlob)
{
	// ファイルパスを検索
	std::wstring fullPath;
	// ファイルパスが存在するか確認
	if (!SearchFilePath(shaderPath.c_str(), fullPath))
	{
		ELOG("Error : Shader file not found. path = %ls", shaderPath.c_str());
		return false;
	}

	// シェーダーファイルを読み込む
	HRESULT hr = D3DReadFileToBlob(fullPath.c_str(), pBlob.GetAddressOf());
	if (FAILED(hr))
	{
		ELOG("Error : D3DReadFileToBlob() Failed. path = %ls, hr = 0x%x", fullPath.c_str(), hr);
		return false;
	}

	return true;
}

//----------------------------------------------------------------------
/// シェーダーを読み込む
//----------------------------------------------------------------------
bool ShaderManager::LoadShader(const std::wstring& shaderName,
	const std::wstring& vsPath,
	const std::wstring& psPath)
{

	// 既に読み込まれているか確認
	auto it = m_LoadedShaders.find(shaderName);
	if (it != m_LoadedShaders.end())
	{
		DLOG("Shader already loaded: %ls", shaderName.c_str());
		return true;  // 既に読み込まれている
	}

	// 新しいシェーダー情報を作成
	ShaderInfo shaderInfo;
	shaderInfo.vsPath = vsPath;
	shaderInfo.psPath = psPath;
	shaderInfo.isValid = false;

	// 頂点シェーダーを読み込む
	if (!LoadShaderFile(vsPath, shaderInfo.pVSBlob))
	{
		ELOG("Error : Failed to load vertex shader. name = %ls, path = %ls",
			shaderName.c_str(), vsPath.c_str());
		return false;
	}

	// ピクセルシェーダーを読み込む
	if (!LoadShaderFile(psPath, shaderInfo.pPSBlob))
	{
		ELOG("Error : Failed to load pixel shader. name = %ls, path = %ls",
			shaderName.c_str(), psPath.c_str());
		return false;
	}

	// 読み込み成功
	shaderInfo.isValid = true;
	m_LoadedShaders[shaderName] = std::move(shaderInfo);

	DLOG("Shader loaded successfully: %ls", shaderName.c_str());
	return true;
}

//----------------------------------------------------------------------
//	ジオメトリシェーダーも含めてシェーダーを読み込む
//----------------------------------------------------------------------
bool ShaderManager::LoadShader(const std::wstring& shaderName, const std::wstring& vsPath, const std::wstring& psPath, const std::wstring& gsPath)
{
	// 既に読み込まれているか確認
	auto it = m_LoadedShaders.find(shaderName);
	if(it != m_LoadedShaders.end())
	{
		DLOG("Shader already loaded: %ls", shaderName.c_str());
		return true;
	}

	// シェーダー情報を作成
	ShaderInfo shaderInfo;
	shaderInfo.vsPath = vsPath;
	shaderInfo.psPath = psPath;
	shaderInfo.gsPath = gsPath;
	shaderInfo.isValid = false;

	// 頂点シェーダーを読み込む
	if(!LoadShaderFile(vsPath, shaderInfo.pVSBlob))
	{
		ELOG("Error : Failed to load vertex shader. name = %ls, path = %ls",
			shaderName.c_str(), vsPath.c_str());
		return false;
	}

	// ピクセルシェーダーを読み込む
	if(!LoadShaderFile(psPath, shaderInfo.pPSBlob))
	{
		ELOG("Error : Failed to load pixel shader. name = %ls, path = %ls",
			shaderName.c_str(), psPath.c_str());
		return false;
	}

	// ジオメトリシェーダーを読み込む
	if(!LoadShaderFile(gsPath, shaderInfo.pGSBlob))
	{
		ELOG("Error : Failed to load geometry shader. name = %ls, path = %ls",
			shaderName.c_str(), gsPath.c_str());
		return false;
	}
	
	// 読み込み成功
	shaderInfo.isValid = true;
	m_LoadedShaders[shaderName] = std::move(shaderInfo);

	DLOG("Shader (VS/GS/PS) loaded successfully: %ls", shaderName.c_str());

	return true;
}

//----------------------------------------------------------------------
//  コンピュートシェーダーを読み込む
//----------------------------------------------------------------------
bool ShaderManager::LoadComputeShader(const std::wstring& shaderName, const std::wstring& csPath)
{
	// 既にロード済みか確認
	auto it = m_LoadedShaders.find(shaderName);

	// ロード済みかどうか
	if (it != m_LoadedShaders.end())
	{
		return true;
	}

	// 新規作成
	ShaderInfo info;
	info.csPath = csPath;
	info.isValid = true;

	// コンピュートシェーダーを読み込む
	if (!LoadShaderFile(csPath, info.pCSBlob))
	{
		ELOG("Error : Failed to load compute shader. name = %ls, path = %ls", shaderName.c_str(), csPath.c_str());
		return false;
	}

	// 読み込み成功
	m_LoadedShaders[shaderName] = std::move(info);

	DLOG("Compute shader loaded: %ls", shaderName.c_str());
	return true;
}

//----------------------------------------------------------------------
/// シェーダー情報を取得
//----------------------------------------------------------------------
ShaderInfo* ShaderManager::GetShader(const std::wstring& shaderName)
{
	// 引数のシェーダーを検索
	auto it = m_LoadedShaders.find(shaderName);
	if (it != m_LoadedShaders.end() && it->second.isValid)
	{
		// pVSBlobとpPSBlobが有効かチェック
		if (it->second.pVSBlob.Get() != nullptr && it->second.pPSBlob.Get() != nullptr)
		{
			return &it->second;
		}
		else
		{
			ELOG("Error : Shader blob is null. shaderName = %ls", shaderName.c_str());
			return nullptr;
		}
	}
	return nullptr;
}

//----------------------------------------------------------------------
//  コンピュートシェーダー情報を取得
//----------------------------------------------------------------------
ShaderInfo* ShaderManager::GetComputeShader(const std::wstring& shaderName)
{
	auto it = m_LoadedShaders.find(shaderName);
	if (it != m_LoadedShaders.end() && it->second.pCSBlob.Get() != nullptr)
		return &it->second;
	return nullptr;
}

//----------------------------------------------------------------------
/// シェーダーが読み込まれているか確認
//----------------------------------------------------------------------
bool ShaderManager::IsShaderLoaded(const std::wstring& shaderName) const
{
	auto it = m_LoadedShaders.find(shaderName);
	return (it != m_LoadedShaders.end() && it->second.isValid);
}

//----------------------------------------------------------------------
/// シェーダーをアンロード
//----------------------------------------------------------------------
void ShaderManager::UnloadShader(const std::wstring& shaderName)
{
	auto it = m_LoadedShaders.find(shaderName);
	if (it != m_LoadedShaders.end())
	{
		// リソース開放
		it->second.pVSBlob;
		it->second.pPSBlob;
		m_LoadedShaders.erase(it);
		DLOG("Shader unloaded: %ls", shaderName.c_str());
	}
}

//----------------------------------------------------------------------
/// 全てのシェーダーをアンロード
//----------------------------------------------------------------------
void ShaderManager::UnloadAll()
{
	for (auto& pair : m_LoadedShaders)
	{
		pair.second.pVSBlob.Reset(); // 頂点シェーダー
		pair.second.pPSBlob.Reset(); // ピクセルシェーダー
		pair.second.pCSBlob.Reset(); // コンピュートシェーダー
	}
	m_LoadedShaders.clear();
	DLOG("All shaders unloaded");
}

//----------------------------------------------------------------------
// 終了処理
//----------------------------------------------------------------------
void ShaderManager::Term()
{
	// 全てのシェーダーをアンロード
	UnloadAll();
}