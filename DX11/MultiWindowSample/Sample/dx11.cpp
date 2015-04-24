#include "stdafx.h"
#include "dx11.h"
#include <d3d11.h>

#include <atlbase.h>
#include <array>

bool					g_InitDX11 = false;
ID3D11Device*			g_pd3dDevice = NULL;
ID3D11DeviceContext*	g_pImmediateContext = NULL;

struct SwapInfo {
	::CComPtr<IDXGISwapChain>			pSwapChain;
	::CComPtr<ID3D11RenderTargetView>	pRenderTargetView;
	D3D11_VIEWPORT						viewport;

	struct SwapInfo()
	:pSwapChain(nullptr)
	,pRenderTargetView(nullptr) {
	};

} g_SwapInfo[2];

namespace {

	HRESULT CreateSwapChain(HWND hwnd, IDXGIFactory* pFactory, SwapInfo& swapInfo, bool inputFullScreen) {

		HRESULT hr = S_OK;

		RECT rc;
		GetClientRect( hwnd, &rc );
		UINT width = rc.right - rc.left;
		UINT height = rc.bottom - rc.top;

		// viewport
		swapInfo.viewport.Width = (FLOAT)width;
		swapInfo.viewport.Height = (FLOAT)height;
		swapInfo.viewport.MinDepth = 0.0f;
		swapInfo.viewport.MaxDepth = 1.0f;
		swapInfo.viewport.TopLeftX = 0;
		swapInfo.viewport.TopLeftY = 0;

		// スワップチェイン設定
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory( &sd, sizeof( sd ) );
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = pFactory->CreateSwapChain(g_pd3dDevice, &sd, &swapInfo.pSwapChain);
		if( FAILED( hr ) ){
			return hr;
		}

	    hr = pFactory->MakeWindowAssociation(hwnd, inputFullScreen ? 0 : DXGI_MWA_NO_ALT_ENTER);
		if( FAILED( hr ) ){
			return hr;
		}

		ID3D11Texture2D* back_buff = NULL;
		hr = swapInfo.pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&back_buff );
		if( FAILED( hr ) ){
			return hr;
		}

		hr = g_pd3dDevice->CreateRenderTargetView( back_buff, NULL, &swapInfo.pRenderTargetView );
		back_buff->Release();
		if( FAILED( hr ) ){
			return hr;
		}

		return hr;
	}

}

//---------------------------------------------------------
HRESULT InitDX11(HWND hwnd0, HWND hwnd1)
{
	if(g_InitDX11)return S_FALSE;

	HRESULT hr = S_OK;

	UINT cdev_flags = 0;
#ifdef _DEBUG
	cdev_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

	hr = D3D11CreateDevice(	NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION,
							&g_pd3dDevice, NULL, &g_pImmediateContext);
	if(FAILED(hr)) {
		return hr;
	}

	::CComPtr<IDXGIDevice1> pDXGI;
	g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**)&pDXGI);

	::CComPtr<IDXGIAdapter> pAdapter;
	pDXGI->GetAdapter(&pAdapter);

	::CComPtr<IDXGIFactory> pFactory;
	pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory);

	// 各スワップチェイン作成
	CreateSwapChain(hwnd0, pFactory, g_SwapInfo[0], false);
	CreateSwapChain(hwnd1, pFactory, g_SwapInfo[1], false);

	g_InitDX11 = true;
	return S_OK;
}

//---------------------------------------------------------
void ExitDX11()
{
	if(!g_InitDX11)return;

	if( g_pImmediateContext ) g_pImmediateContext->ClearState();

	if( g_pImmediateContext ) g_pImmediateContext->Release();
	if( g_pd3dDevice ) g_pd3dDevice->Release();
	
	g_InitDX11 = false;
}

//---------------------------------------------------------
void RenderDX11()
{
	for(int i = 0; i < _countof(g_SwapInfo); ++i) {
		ID3D11RenderTargetView* pRTV = g_SwapInfo[i].pRenderTargetView;
		g_pImmediateContext->OMSetRenderTargets( 1, &pRTV, NULL );
		g_pImmediateContext->RSSetViewports( 1, &g_SwapInfo[i].viewport );

	    // 指定色で画面クリア
		float clearColor[4];
		if(i == 0) {
			static float fR = 0.0f;
			if(fR < 1.0f)	fR += 0.001f;
			else			fR = 0.0f;
			clearColor[0] = fR;
			clearColor[1] = 0.0f;
			clearColor[2] = 0.0f;
			clearColor[3] = 1.0f;
		}
		else {
			static float fG = 0.0f;
			if(fG < 1.0f)	fG += 0.001f;
			else			fG = 0.0f;
			clearColor[0] = 0.0f;
			clearColor[1] = fG;
			clearColor[2] = 0.0f;
			clearColor[3] = 1.0f;
		}
	    g_pImmediateContext->ClearRenderTargetView( g_SwapInfo[i].pRenderTargetView, clearColor );

		//結果をウインドウに反映
	    g_SwapInfo[i].pSwapChain->Present( 1, 0 );
	}
}

void SwitchScreen() {
	BOOL bFullScreen;
	g_SwapInfo[0].pSwapChain->GetFullscreenState(&bFullScreen, nullptr);
	if(bFullScreen) {
		g_SwapInfo[0].pSwapChain->SetFullscreenState(FALSE, nullptr);
		g_SwapInfo[1].pSwapChain->SetFullscreenState(FALSE, nullptr);
	}
	else {
		g_SwapInfo[0].pSwapChain->SetFullscreenState(TRUE, nullptr);
		g_SwapInfo[1].pSwapChain->SetFullscreenState(TRUE, nullptr);
	}
}

