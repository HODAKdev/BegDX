#include <windows.h>
#include <d3d11.h>
#include <string>
#pragma comment(lib, "d3d11.lib")
#define WIDTH 1280
#define HEIGHT 720

static HWND                    hwnd = NULL;
static HRESULT                 hr = S_OK;
static DXGI_FORMAT             dxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
static D3D_DRIVER_TYPE         dxDriverType = D3D_DRIVER_TYPE_HARDWARE;
static D3D_FEATURE_LEVEL       dxFeatureLevel = D3D_FEATURE_LEVEL_11_0;
static IDXGISwapChain*         dxSwapChain = NULL;
static ID3D11Device*           dxDevice = NULL;
static ID3D11DeviceContext*    dxDeviceContext = NULL;
static ID3D11RenderTargetView* dxRenderTargetView = NULL;
static ID3D11Texture2D*        dxDepthStencil = NULL;
static ID3D11DepthStencilView* dxDepthStencilView = NULL;

bool DX11CreateWindow(HINSTANCE hInstance, std::wstring name, UINT width, UINT height);
bool DX11CreateContext();
void DX11WindowUpdate();
void DX11WindowProcess();
bool DX11WindowResize();
void DX11WindowShutdown();
LRESULT CALLBACK DX11WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
bool DX11CreateRenderTargetView();
bool DX11CreateDepthStencilView();
UINT DX11GetContextWidth();
UINT DX11GetContextHeight();

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	if (!DX11CreateWindow(hInstance, L"HODAK", WIDTH, HEIGHT))
		return 0;

	if (!DX11CreateContext())
		return 0;

	ShowWindow(hwnd, nCmdShow);
	DX11WindowUpdate();
	DX11WindowShutdown();

	return 0;
}

bool DX11CreateWindow(HINSTANCE hInstance, std::wstring name, UINT width, UINT height)
{
	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = DX11WindowProc;
	wcex.cbClsExtra = NULL;
	wcex.cbWndExtra = NULL;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(32, 32, 32));
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = name.c_str();
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wcex))
		return false;

	//////////////////////////////

	UINT x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
	UINT y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

	hwnd = CreateWindowEx(
		NULL,
		name.c_str(),
		name.c_str(),
		WS_OVERLAPPEDWINDOW,
		x, y,
		width, height,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (!hwnd)
		return false;

	//////////////////////////////

	return true;
}

LRESULT CALLBACK DX11WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_EXITSIZEMOVE:
		DX11WindowResize();
		break;
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 256;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 256;
		break;
	case WM_SIZE:
		if (SIZE_MAXIMIZED == wParam)
			DX11WindowResize();
		if (SIZE_RESTORED == wParam)
			DX11WindowResize();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

void DX11WindowUpdate()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DX11WindowProcess();
		}
	}
}

void DX11WindowProcess()
{
	static float ClearColor[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
	dxDeviceContext->ClearRenderTargetView(dxRenderTargetView, ClearColor);
	dxDeviceContext->ClearDepthStencilView(dxDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
	dxDeviceContext->OMSetRenderTargets(1, &dxRenderTargetView, dxDepthStencilView);

	{
		// code
	}

	dxSwapChain->Present(0, 0);
}

bool DX11CreateContext()
{
	DXGI_SWAP_CHAIN_DESC dx;
	ZeroMemory(&dx, sizeof(DXGI_SWAP_CHAIN_DESC));

	dx.BufferCount = 2;
	dx.BufferDesc.Format = dxFormat;
	dx.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dx.OutputWindow = hwnd;
	dx.SampleDesc.Count = 1;
	dx.SampleDesc.Quality = 0;
	dx.Windowed = TRUE;
	dx.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	UINT flags = 0;

#if defined(_DEBUG)
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		dxDriverType,
		NULL,
		flags,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&dx,
		&dxSwapChain,
		&dxDevice,
		&dxFeatureLevel,
		&dxDeviceContext);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	if (!DX11CreateRenderTargetView())
		return false;

	//////////////////////////////

	if (!DX11CreateDepthStencilView())
		return false;

	//////////////////////////////

	return true;
}

bool DX11CreateRenderTargetView()
{
	ID3D11Texture2D* backBuffer = nullptr;
	hr = dxSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	hr = dxDevice->CreateRenderTargetView(backBuffer, NULL, &dxRenderTargetView);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	backBuffer->Release();
	return true;
}

bool DX11CreateDepthStencilView()
{
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));

	descDepth.Width = DX11GetContextWidth();
	descDepth.Height = DX11GetContextHeight();
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = dxDevice->CreateTexture2D(&descDepth, NULL, &dxDepthStencil);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));

	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = dxDevice->CreateDepthStencilView(dxDepthStencil, &descDSV, &dxDepthStencilView);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	return true;
}

bool DX11WindowResize()
{
	dxDeviceContext->OMSetRenderTargets(0, 0, 0);
	dxRenderTargetView->Release();
	dxDepthStencil->Release();
	dxDepthStencilView->Release();

	hr = dxSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	if (FAILED(hr))
		return false;

	//////////////////////////////

	if (!DX11CreateRenderTargetView())
		return false;

	//////////////////////////////

	if (!DX11CreateDepthStencilView())
		return false;

	//////////////////////////////

	return true;
}

UINT DX11GetContextWidth()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT width = rc.right - rc.left;
	return width;
}
UINT DX11GetContextHeight()
{
	RECT rc;
	GetClientRect(hwnd, &rc);
	UINT height = rc.bottom - rc.top;
	return height;
}

void DX11WindowShutdown()
{
	if (dxSwapChain)
		dxSwapChain->Release();

	if (dxDevice)
		dxDevice->Release();

	if (dxDeviceContext)
		dxDeviceContext->Release();

	if (dxRenderTargetView)
		dxRenderTargetView->Release();

	if (dxDepthStencil)
		dxDepthStencil->Release();

	if (dxDepthStencilView)
		dxDepthStencilView->Release();
}