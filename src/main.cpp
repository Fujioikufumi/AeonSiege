#if defined(DEBUG) || defined(_debug)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif//defined(DEBUG) || defined(_DEBUG)

//--------------------------------------------------------------
// Include
//--------------------------------------------------------------
#include "GameApp.h"
#include "NameSpace.h"
//--------------------------------------------------------------
// メインエントリーポイント
//--------------------------------------------------------------
int wmain(int argc, wchar_t** argv, wchar_t** evnp)
{
#if defined(DEBUG) || defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif// defined(DEBUG) || defined(_DEBUG)
	// アプリケーションを実行
	GameApp(SCREEN_WIDTH, SCREEN_HEIGHT).Run();
	return 0;
}