#include <Windows.h>
#include <tchar.h>
#include <vector>
#include <string>
#ifdef _DEBUG
#include <iostream>
#endif

/*
TODO 進行度
画面クリア処理を完了しました。
97P
*/

//! DirectX関連インクルード
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

//! 追加インクルード
#include<wrl/client.h>

using namespace std;
using namespace Microsoft::WRL;
using namespace DirectX;

const LONG g_windWidth = 1280, g_windHeight = 800;

ComPtr<ID3D12Device> g_pDevice = nullptr;
ComPtr<IDXGIFactory6> g_pDxgiFactory = nullptr;
ComPtr<IDXGISwapChain4> g_pSwapchain = nullptr;

vector<ComPtr<ID3D12Resource>> g_pbackBuffers;

//? コマンドリスト
ComPtr<ID3D12GraphicsCommandList> g_pCmdList = nullptr;
ComPtr<ID3D12CommandAllocator> g_pCmdAllocator = nullptr;
ComPtr<ID3D12CommandQueue> g_pCmdQueue = nullptr;

//? ディスクリプタ
ComPtr<ID3D12DescriptorHeap> g_prtvHeaps = nullptr;

//? フェンス
ComPtr<ID3D12Fence> g_pFence = nullptr;
UINT64 g_fenceVal = 0;

//? ベクトル構造体
XMFLOAT3 vertices[] =
{
	{-1.0f,-1.0f,0.0f},
	{-1.0f,1.0f,0.0f} ,
	{1.0f,-1.0f,0.0f}
};

//----------------------------------------------------------------------------
//デバック用関数
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

//デバックレイヤー
void EnableDebugLayer()
{
	ID3D12Debug* debugLayer = nullptr;
	auto result = D3D12GetDebugInterface(IID_PPV_ARGS(&debugLayer));

	debugLayer->EnableDebugLayer();//デバックレイヤーを有効化
	debugLayer->Release();//有効化したらインターフェイスを解放
}

//----------------------------------------------------------------------------
//Direct3D12
//----------------------------------------------------------------------------

bool InitDirect3D()
{
#ifdef _DEBUG
	//デバックレイヤーを有効化
	EnableDebugLayer();

	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG,IID_PPV_ARGS(g_pDxgiFactory.GetAddressOf()))))
		return false;
#else
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(g_pDxgiFactory.GetAddressOf()))))
		return false;
#endif // _DEBUG

	//アダプターを指定する
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

	//Direct3Dデバイスの作成
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

	//作成失敗
	if (g_pDevice == nullptr)
	{
		return false;
	}

	return true;
}

//! 画面色のクリア
bool ClearWindow(HWND hwnd)
{
	//コマンドリストの作成とコマンドアロケータ
	HRESULT result = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCmdAllocator));

	if (result != S_OK)
		return false;

	result = g_pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_pCmdAllocator.Get(),nullptr,IID_PPV_ARGS(&g_pCmdList));
	if (result != S_OK)
		return false;

	//コマンドキューの作成
	D3D12_COMMAND_QUEUE_DESC cmdQueueDesc = {};

	//タイムアウト無し
	cmdQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE;

	//アダプターを一つ以上使わない場合は０で良い
	cmdQueueDesc.NodeMask = 0;

	//プライオリティは特に設定なし
	cmdQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	//コマンドリストに合わせる
	cmdQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	//キューを作成
	result = g_pDevice->CreateCommandQueue(&cmdQueueDesc, IID_PPV_ARGS(&g_pCmdQueue));
	if (result != S_OK)
		return false;

	//スワップチェーンを作成
	DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};

	swapchainDesc.Width = g_windWidth;
	swapchainDesc.Height = g_windHeight;
	swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchainDesc.Stereo = false;
	swapchainDesc.SampleDesc.Count = 1;
	swapchainDesc.SampleDesc.Quality = 0;
	swapchainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
	swapchainDesc.BufferCount = 2;

	//バッファは伸び縮み可能
	swapchainDesc.Scaling = DXGI_SCALING_STRETCH;

	//フリップ後は速やかに破棄
	swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	//特に指定なし
	swapchainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	//ウィンドウ⇔フルスクリーン切り替え可能
	swapchainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	result = g_pDxgiFactory->CreateSwapChainForHwnd(g_pCmdQueue.Get(), hwnd, &swapchainDesc, nullptr, nullptr, (IDXGISwapChain1**)g_pSwapchain.GetAddressOf());
	if (result != S_OK)
		return false;

	//TODO 学習メモ
	/*
		2021/01/29 0:58
		スワップチェイン用のバッファ作成まで完了。
		次の目標＝＞レンダーターゲットビューの作成をする
	*/
	
	//ディスクリプタヒープを作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.NodeMask = 0;
	heapDesc.NumDescriptors = 2;//表裏の二つ
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAGS::D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	result = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_prtvHeaps));

	//スワップチェーンと紐づける
	DXGI_SWAP_CHAIN_DESC swcDesc = {};
	result = g_pSwapchain->GetDesc(&swcDesc);

	g_pbackBuffers.resize(swcDesc.BufferCount);

	//先頭のアドレスを得る
	D3D12_CPU_DESCRIPTOR_HANDLE handle = g_prtvHeaps->GetCPUDescriptorHandleForHeapStart();

	for (UINT idx = 0; idx < swcDesc.BufferCount; idx++)
	{
		result = g_pSwapchain->GetBuffer(idx, IID_PPV_ARGS(&g_pbackBuffers[idx]));

		//レンダーターゲットビューを生成する
		g_pDevice->CreateRenderTargetView(g_pbackBuffers[idx].Get(), nullptr, handle);

		//ポインターをずらす
		handle.ptr += g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	result = g_pDevice->CreateFence(g_fenceVal, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));

	return true;
}

void Update()
{
	HRESULT result = g_pCmdAllocator->Reset();

	auto bbIdx = g_pSwapchain->GetCurrentBackBufferIndex();

	D3D12_RESOURCE_BARRIER BarrierDesc = {};

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;//遷移
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAGS::D3D12_RESOURCE_BARRIER_FLAG_NONE;//指定なし
	BarrierDesc.Transition.pResource = g_pbackBuffers[bbIdx].Get();
	BarrierDesc.Transition.Subresource = 0;

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PRESENT;//直前PRESENT状態
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_RENDER_TARGET;//今からレンダーターゲット状態

	g_pCmdList->ResourceBarrier(1, &BarrierDesc);//バリア指定実行

	//レンダーターゲットを指定
	auto rtvH = g_prtvHeaps->GetCPUDescriptorHandleForHeapStart();
	rtvH.ptr += bbIdx * g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_pCmdList->OMSetRenderTargets(1, &rtvH, true, nullptr);

	//画面クリア
	const float clearColor[] = { 1.0f,1.0f,0.0f,1.0f };//黄色
	g_pCmdList->ClearRenderTargetView(rtvH, clearColor, 0, nullptr);

	BarrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	BarrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	g_pCmdList->ResourceBarrier(1, &BarrierDesc);

	//命令をクローズ
	g_pCmdList->Close();


	//コマンドリストの実行
	ID3D12CommandList* _cmdLists[] = { g_pCmdList.Get() };

	g_pCmdQueue->ExecuteCommandLists(1, _cmdLists);

	g_pCmdQueue->Signal(g_pFence.Get(), ++g_fenceVal);
	
	if (g_pFence->GetCompletedValue() != g_fenceVal)
	{
		//イベントハンドルの取得
		auto event = CreateEvent(nullptr, false, false, nullptr);

		g_pFence->SetEventOnCompletion(g_fenceVal, event);

		//イベントが発生するまで待つ
		WaitForSingleObject(event, INFINITE);

		//イベントをハンドルを閉じる
		CloseHandle(event);
	}

	//キューをクリア
	g_pCmdAllocator->Reset();
	g_pCmdList->Reset(g_pCmdAllocator.Get(), nullptr);

	//画面のスワップ
	g_pSwapchain->Present(1, 0);

	//TODO 学習メモ
	/*
		2021/02/01
		画面クリアまで、完了
		90P＝＞エラー処理を作成
	*/
}

//----------------------------------------------------------------------------
//ウィンドウ生成
//----------------------------------------------------------------------------

// -- ウィンドウプロシージャ --
//! コールバック関数
LRESULT WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
		// キーが押された
		switch (wParam) {
		case VK_ESCAPE:
			//ウインドウを破棄する
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

	// -- ウィンドウの生成＆登録 --
	WNDCLASSEX window = {};
	window.cbSize = sizeof(WNDCLASSEX);
	window.lpfnWndProc = (WNDPROC)WindowProcedure;
	window.lpszClassName = _T("Dx12Window");
	window.hInstance = GetModuleHandle(nullptr);

	RegisterClassEx(&window);

	RECT windRect = { 0,0,g_windWidth,g_windHeight };

	AdjustWindowRect(&windRect, WS_OVERLAPPEDWINDOW, false);

	// -- ウィンドウオブジェクト生成 --
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
		ClearWindow(hWnd);
	}

	MSG msg = {};

	while (true)
	{
		Update();

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