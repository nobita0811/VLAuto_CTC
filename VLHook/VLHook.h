#pragma once
/*


#ifdef HOOKPR_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

HOOKDLL_API unsigned StartInject(HWND hWndGame, HWND hWndAuto);
HOOKDLL_API unsigned StopInject(HWND hWndGame);
*/
#ifdef HOOKPR_EXPORTS
#define HOOKDLL_API __declspec(dllexport)
#else
#define HOOKDLL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	HOOKDLL_API unsigned __stdcall StartInject(HWND hWndGame, HWND hWndAuto);
	HOOKDLL_API unsigned __stdcall StopInject(HWND hWndGame);

#ifdef __cplusplus
}
#endif