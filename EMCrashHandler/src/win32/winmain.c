#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(disable: 28251)		// disables windows annotations warning

static void WindowsError(void)
{
	LPVOID wlpmsgbuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&wlpmsgbuf,
		0,
		NULL
	);

	MessageBox(NULL, wlpmsgbuf, L"GetLastError", MB_OK | MB_ICONINFORMATION);
	LocalFree(wlpmsgbuf);
}

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, PWSTR pcmdline, int ncmdshow)
{
	HANDLE mapfile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"EMCrashHandlerFileMapping");
	if (mapfile)
	{
		WindowsError();
		return(1);
	}

	return(0);
}
