#include <wchar.h>
#include <windows.h>

static int GetExeDirW(wchar_t *outDir, DWORD size) {
	DWORD len = GetModuleFileNameW(NULL, outDir, size);
	if (len == 0 || len == size) return 0;

	for (DWORD i = len; i > 0; --i) {
		if (outDir[i] == L'\\') {
			outDir[i + 1] = 0;
			return 1;
		}
	}
	return 0;
}

static int WriteRegistry() {
	wchar_t exeDir[MAX_PATH];
	wchar_t principiaPath[MAX_PATH * 2];

	if (!GetExeDirW(exeDir, MAX_PATH))
		return 0;

	swprintf(principiaPath, MAX_PATH * 2, L"\"%sprincipia.exe\" %%1", exeDir);

	HKEY hKey;

	LONG res = RegCreateKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Classes\\principia",
		0,
		NULL,
		0,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL
	);

	if (res != ERROR_SUCCESS)
		return 0;

	const wchar_t *protocol = L"URL:Principia";
	const wchar_t *empty = L"";

	RegSetValueExW(hKey, NULL, 0, REG_SZ, (const BYTE*)protocol,
		(DWORD)((wcslen(protocol) + 1) * sizeof(wchar_t)));

	RegSetValueExW(hKey, L"URL Protocol", 0, REG_SZ, (const BYTE*)empty,
		(DWORD)sizeof(wchar_t));

	HKEY hSub;

	RegCreateKeyExW(hKey, L"shell\\open\\command",
		0, NULL, 0, KEY_WRITE, NULL, &hSub, NULL);

	RegSetValueExW(hSub, NULL, 0, REG_SZ,
		(const BYTE*)principiaPath,
		(DWORD)((wcslen(principiaPath) + 1) * sizeof(wchar_t)));

	RegCloseKey(hSub);
	RegCloseKey(hKey);

	return 1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	int ret = MessageBoxW(
		NULL,
		L"Register a principia:// protocol handler for this user? This will make changes in the Windows Registry.",
		L"Register Principia Protocol Handler",
		MB_YESNO | MB_ICONQUESTION
	);

	if (ret == IDYES) {
		if (WriteRegistry())
			MessageBoxW(NULL, L"Protocol registered.", L"Success", MB_OK | MB_ICONINFORMATION);
		else
			MessageBoxW(NULL, L"Failed to register protocol.", L"Error", MB_OK | MB_ICONERROR);
	}

	return 0;
}
