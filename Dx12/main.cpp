#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <string>
#ifdef _DEBUG
#include <iostream>
#endif

//! DirectX�֘A�C���N���[�h
#include <d3d12.h>
#include <dxgi1_6.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

//! �ǉ��C���N���[�h
#include<wrl/client.h>

using namespace std;
using namespace Microsoft::WRL;

float g_windWidth = 1280, g_windHeight = 800;

ComPtr<ID3D12Device> g_pDevice = nullptr;
ComPtr<IDXGIFactory6> g_pDxgiFactory = nullptr;
ComPtr<IDXGISwapChain4> g_pSwapchain = nullptr;

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
//Direct3D12
//----------------------------------------------------------------------------

bool InitDirect3D()
{
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(g_pDxgiFactory.GetAddressOf()))))
		return false;

	//�A�_�v�^�[���w�肷��
	vector<ComPtr<IDXGIAdapter>> adapters;

	ComPtr<IDXGIAdapter> tempAdapter = nullptr;

	for (int i = 0; g_pDxgiFactory->EnumAdapters(i, tempAdapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; i++)
	{
		adapters.push_back(tempAdapter);
	}

	for (auto adamp: adapters)
	{
		DXGI_ADAPTER_DESC adesc = {};
		adamp->GetDesc(&adesc);

		wstring strDesc = adesc.Description;

		if (strDesc.find(L"NVIDIA") != string::npos)
		{
			tempAdapter = adamp;
			break;
		}
	}

	//Direct3D�f�o�C�X�̍쐬
	const D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
	};

	D3D_FEATURE_LEVEL featurelevel;

	for (auto lv : levels)
	{
		if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), lv, IID_PPV_ARGS(g_pDevice.GetAddressOf()))))
		{
			featurelevel = lv;
			break;
		}
	}

	//�쐬���s
	if (g_pDevice == nullptr)
	{
		return false;
	}

	return true;
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

	if (InitDirect3D())
	{
		
	}

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