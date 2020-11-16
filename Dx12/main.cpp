#include <Windows.h>
#include <tchar.h>
#ifdef _DEBUG
#include <iostream>
#endif

using namespace std;

float g_windWidth = 1280, g_windHeight = 800;

//----------------------------------------------------------------------------
//�f�o�b�N�p�֐�
//----------------------------------------------------------------------------

void DebugOut(const char* format, ...)
{
#ifdef _DEBUG
	va_list valist;
	va_start(valist, format);
	printf_s(format, valist);
	va_end(valist);
#endif // _DEBUG
}

//----------------------------------------------------------------------------
//�E�B���h�E����
//----------------------------------------------------------------------------

// -- �E�B���h�E�v���V�[�W�� --
//! �R�[���o�b�N�֐�
LRESULT WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		// �L�[�������ꂽ
		switch (wParam) {
		case VK_ESCAPE:
			//�E�C���h�E��j������
			DestroyWindow(hWnd);
			break;
		}
		break;
	default:
		break;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

#ifdef _DEBUG
int main()
{
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif // _DEBUG

	// -- �E�B���h�E�̐������o�^ --
	WNDCLASSEX window = {};
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure;
	window.lpszClassName = _T("Dx12Window");
	window.hInstance = GetModuleHandle(nullptr);

	RegisterClassEx(&window);

	RECT windRect = { 0,0,g_windWidth,g_windHeight };

	AdjustWindowRect(&windRect, WS_OVERLAPPEDWINDOW, false);

	// -- �E�B���h�E�I�u�W�F�N�g���� --
	HWND hWnd = CreateWindow(window.lpszClassName,
		_T("Dx12"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windRect.right - windRect.left,
		windRect.bottom - windRect.top,
		nullptr,
		nullptr,
		window.hInstance,
		nullptr);

	ShowWindow(hWnd, SW_SHOW);

	MSG msg = {};

	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			break;
		}
	}

	UnregisterClass(window.lpszClassName, window.hInstance);

	return 0;
}