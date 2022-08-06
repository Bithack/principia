#include "windows.h"
#include "stdio.h"
#include "stdlib.h"
#include "shlwapi.h"
#include <sys/stat.h>
#include <direct.h>

inline bool file_exists(const char *name)
{
    struct stat buffer;
    return (stat(name, &buffer) == 0);
}

HRESULT RegGetString(HKEY hKey, LPCTSTR szValueName, LPTSTR * lpszResult) {
 
    // Given a HKEY and value name returns a string from the registry.
    // Upon successful return the string should be freed using free()
    // eg. RegGetString(hKey, TEXT("my value"), &szString);
 
    DWORD dwType=0, dwDataSize=0, dwBufSize=0;
    LONG lResult;
 
    // Incase we fail set the return string to null...
    if (lpszResult != NULL) *lpszResult = NULL;
 
    // Check input parameters...
    if (hKey == NULL || lpszResult == NULL) return E_INVALIDARG;
 
    // Get the length of the string in bytes (placed in dwDataSize)...
    lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, NULL, &dwDataSize );
 
    // Check result and make sure the registry value is a string(REG_SZ)...
    if (lResult != ERROR_SUCCESS) return HRESULT_FROM_WIN32(lResult);
    else if (dwType != REG_SZ)    return DISP_E_TYPEMISMATCH;
 
    // Allocate memory for string - We add space for a null terminating character...
    dwBufSize = dwDataSize + (1 * sizeof(TCHAR));
    *lpszResult = (LPTSTR)malloc(dwBufSize);
 
    if (*lpszResult == NULL) return E_OUTOFMEMORY;
 
    // Now get the actual string from the registry...
    lResult = RegQueryValueEx(hKey, szValueName, 0, &dwType, (LPBYTE) *lpszResult, &dwDataSize );
 
    // Check result and type again.
    // If we fail here we must free the memory we allocated...
    if (lResult != ERROR_SUCCESS) { free(*lpszResult); return HRESULT_FROM_WIN32(lResult); }
    else if (dwType != REG_SZ)    { free(*lpszResult); return DISP_E_TYPEMISMATCH; }
 
    // We are not guaranteed a null terminated string from RegQueryValueEx.
    // Explicitly null terminate the returned string...
    (*lpszResult)[(dwBufSize / sizeof(TCHAR)) - 1] = TEXT('\0');
 
    return NOERROR;
}

int main(int argc, char **argv)
{
    DWORD hr;
    HKEY hKey         = NULL;
    LONG lResult;
    LPTSTR regedit_dir = 0;
    TCHAR current_dir[MAX_PATH];
    bool got_regedit_path = false;
    char path[1024];
    bool bin_exists = false;

    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Bithack\\Principia", 0, KEY_QUERY_VALUE, &hKey);
    if (lResult == ERROR_SUCCESS) {
        hr = RegGetString(hKey, TEXT(""), &regedit_dir);
        if (!FAILED(hr)) {
            got_regedit_path = true;
        }
    }
    
    if (got_regedit_path) {
        snprintf(path, 1024, "%s\\principia.exe", regedit_dir);
        bin_exists = file_exists(path);
    }

    if (!bin_exists) {
        /* Fallback to the current directory */
        GetModuleFileName(NULL, current_dir, MAX_PATH);
        PathRemoveFileSpec(current_dir);
        char path[1024];
        snprintf(path, 1024, "%s\\principia.exe", current_dir);
        bin_exists = file_exists(path);
    }

    if (bin_exists) {
        char dir[1050];
        strcpy(dir, path);
        PathRemoveFileSpec(dir);
        printf("Found binary! %s(in %s)\n", path, dir);
        if (argc >= 2) {
            printf("arg 2: %s", argv[1]);
            ShellExecute(NULL, "open", path, argv[1], dir, SW_SHOWNORMAL);
        } else {
            /* No argument given, just open the game */
            ShellExecute(NULL, "open", path, NULL, dir, SW_SHOWNORMAL);
        }

    } else {
        printf("No binary found\n");
    }
    
    return 0;
}
