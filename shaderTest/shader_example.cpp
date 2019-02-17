// include the basic windows header files and the Direct3D header files
#include <stdafx.h>
#include <DirectXMath.h>
#include <stdio.h>
#include <windows.h>
#include <windowsx.h>
#include <stdint.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10.h>
#include <d3dcompiler.h>
#include <shellapi.h>

using namespace DirectX;

#define M_PI       3.14159265358979323846
// include the Direct3D Library file
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dx11.lib")
#pragma comment (lib, "d3dx10.lib")

// define the screen resolution
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 600
#define F3 sizeof(float) * 3
#define F2 sizeof(float) * 2
#define F4 sizeof(float) * 4
#define F 2

struct CBViewProjection;
struct GInstanceBuffer;

// global declarations
IDXGISwapChain *swapchain;             // the pointer to the swap chain interface
ID3D11Device *dev;                     // the pointer to our Direct3D device interface
ID3D11DeviceContext *devcon;           // the pointer to our Direct3D device context
ID3D11RenderTargetView *backbuffer;    // the pointer to our back buffer
ID3D11InputLayout *pLayout;            // the pointer to the input layout
ID3D11VertexShader *pVS;               // the pointer to the vertex shader
ID3D11PixelShader *pPS;                // the pointer to the pixel shader
ID3D11Buffer *pVBuffer;                // the pointer to the vertex buffer
ID3D11Buffer *pCBuffer;                // the pointer to the cbuffer
CBViewProjection* pvsConstData;
GInstanceBuffer* pgInstanceBuffer;

#define row_major
typedef XMFLOAT4X4A float4x4;
typedef XMFLOAT3X4A float3x4;
typedef XMFLOAT3X3 float3x3;
typedef XMFLOAT4A float4;
typedef XMFLOAT3A float3;
typedef XMFLOAT2A float2;
typedef uint32_t uint;
struct GInstanceBuffer {
	row_major float3x4 wmat;       // Offset:    0
    row_major float3x3 wmatI;      // Offset:   48
    float4 color;                  // Offset:   84
    uint matirxIndex;              // Offset:  100
    uint matirxIndexPF;            // Offset:  104
    float padding;
};
struct CBViewProjection
{
   
   row_major float4x4 fViewProj;      // Offset:    0 Size:    64
   row_major float4x4 fView;          // Offset:   64 Size:    64 [unused]
   row_major float4x4 fProj;          // Offset:  128 Size:    64 [unused]
   row_major float4x4 fViewI;         // Offset:  192 Size:    64 [unused]
   row_major float4x4 fProjI;         // Offset:  256 Size:    64 [unused]
   row_major float4x4 fViewProjI;     // Offset:  320 Size:    64 [unused]
   float3 fCameraPos;                 // Offset:  384 Size:    12 [unused]
   float3 fCameraDir;                 // Offset:  400 Size:    12 [unused]
   float3 fZToLinear;                 // Offset:  416 Size:    12 [unused]
   float fCameraNearClip;             // Offset:  428 Size:     4 [unused]
   float fCameraFarClip;              // Offset:  432 Size:     4 [unused]
   float fCameraTargetDist;           // Offset:  436 Size:     4 [unused]
   float4 fPassThrough;               // Offset:  448 Size:    16 [unused]
   float3 fLODBasePos;                // Offset:  464 Size:    12 [unused]
   row_major float4x4 fViewProjPF;    // Offset:  480 Size:    64 [unused]
   row_major float4x4 fViewProjIPF;   // Offset:  544 Size:    64 [unused]
   row_major float4x4 fViewPF;        // Offset:  608 Size:    64 [unused]
   row_major float4x4 fProjPF;        // Offset:  672 Size:    64 [unused]
   row_major float4x4 fViewProjIViewProjPF;// Offset:  736 Size:    64 [unused]
   row_major float4x4 fNoJitterProj;  // Offset:  800 Size:    64 [unused]
   row_major float4x4 fNoJitterViewProj;// Offset:  864 Size:    64 [unused]
   row_major float4x4 fNoJitterViewProjI;// Offset:  928 Size:    64 [unused]
   row_major float4x4 fNoJitterViewProjIViewProjPF;// Offset:  992 Size:    64 [unused]
   float2 fPassThroughCorrect;        // Offset: 1056 Size:     8 [unused]
   bool bWideMonitor;                 // Offset: 1064 Size:     4 [unused]

};

// a struct to define a single vertex
struct VERTEX{
	FLOAT X, Y, Z;
	FLOAT NX, NY, NZ;
	FLOAT TX, TY, TZ, TW;
	FLOAT UX, UY;
	FLOAT U2X, U2Y;
	D3DXCOLOR Color;
	FLOAT PX, PY, PZ;
	uint32_t iid;
};

// function prototypes
void InitD3D(HWND hWnd);    // sets up and initializes Direct3D
void RenderFrame(void);     // renders a single frame
void CleanD3D(void);        // closes Direct3D and releases memory
void InitGraphics(void);    // creates the shape to render
void InitPipeline(void);    // loads and prepares the shaders
ID3D10Blob* loadfile(const char* filePath);

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine,
                   int nCmdShow)
{
    HWND hWnd;
    WNDCLASSEX wc;

    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";

    RegisterClassEx(&wc);

    RECT wr = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    hWnd = CreateWindowEx(NULL,
                          L"WindowClass",
                          L"Our First Direct3D Program",
                          WS_OVERLAPPEDWINDOW,
                          300,
                          300,
                          wr.right - wr.left,
                          wr.bottom - wr.top,
                          NULL,
                          NULL,
                          hInstance,
                          NULL);

    ShowWindow(hWnd, nCmdShow);

    // set up and initialize Direct3D
    InitD3D(hWnd);

    // enter the main loop:

    MSG msg;

    while(TRUE)
    {
        if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if(msg.message == WM_QUIT)
                break;
        }

        RenderFrame();
    }

    // clean up DirectX and COM
    CleanD3D();

    return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
        case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            } break;
    }

    return DefWindowProc (hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void InitD3D(HWND hWnd)
{
    // create a struct to hold information about the swap chain
    DXGI_SWAP_CHAIN_DESC scd;

    // clear out the struct for use
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    // fill the swap chain description struct
    scd.BufferCount = 1;                                   // one back buffer
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // use 32-bit color
    scd.BufferDesc.Width = SCREEN_WIDTH;                   // set the back buffer width
    scd.BufferDesc.Height = SCREEN_HEIGHT;                 // set the back buffer height
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;     // how swap chain is to be used
    scd.OutputWindow = hWnd;                               // the window to be used
    scd.SampleDesc.Count = 4;                              // how many multisamples
    scd.Windowed = TRUE;                                   // windowed/full-screen mode
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;    // allow full-screen switching

    // create a device, device context and swap chain using the information in the scd struct
    D3D11CreateDeviceAndSwapChain(NULL,
                                  D3D_DRIVER_TYPE_HARDWARE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  D3D11_SDK_VERSION,
                                  &scd,
                                  &swapchain,
                                  &dev,
                                  NULL,
                                  &devcon);


    // get the address of the back buffer
    ID3D11Texture2D *pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    // use the back buffer address to create the render target
    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    // set the render target as the back buffer
    devcon->OMSetRenderTargets(1, &backbuffer, NULL);


    // Set the viewport
    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = SCREEN_WIDTH;
    viewport.Height = SCREEN_HEIGHT;

    devcon->RSSetViewports(1, &viewport);

    InitPipeline();
    InitGraphics();
}


// this is the function used to render a single frame
void RenderFrame(void)
{
    // clear the back buffer to a deep blue
    devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));

        // select which vertex buffer to display
        UINT stride = sizeof(VERTEX);
        UINT offset = 0;
        devcon->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

        // select which primtive type we are using
        devcon->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // draw the vertex buffer to the back buffer
        devcon->Draw(3, 0);

    // switch the back buffer and the front buffer
    swapchain->Present(0, 0);
}


// this is the function that cleans up Direct3D and COM
void CleanD3D(void)
{
    swapchain->SetFullscreenState(FALSE, NULL);    // switch to windowed mode

    // close and release all existing COM objects
    pLayout->Release();
    pVS->Release();
    pPS->Release();
    pVBuffer->Release();
    swapchain->Release();
    backbuffer->Release();
    dev->Release();
    devcon->Release();
}

D3DXMATRIX
ProjectionMatrix(const float near_plane, // Distance to near clipping 
										 // plane
	const float far_plane,  // Distance to far clipping 
							// plane
	const float fov_horiz,  // Horizontal field of view 
							// angle, in radians
	const float fov_vert)   // Vertical field of view 
							// angle, in radians
{
	float    h, w, Q;

	w = (float)1 / tan(fov_horiz*0.5);  // 1/tan(x) == cot(x)
	h = (float)1 / tan(fov_vert*0.5);   // 1/tan(x) == cot(x)
	Q = far_plane / (far_plane - near_plane);

	D3DXMATRIX ret;
	ZeroMemory(&ret, sizeof(ret));

	ret(0, 0) = w;
	ret(1, 1) = h;
	ret(2, 2) = Q;
	ret(3, 2) = -Q * near_plane;
	ret(2, 3) = 1;
	return ret;
}   // End of ProjectionMatrix
// this is the function that creates the shape to render
void InitGraphics()
{
    // create a triangle using the VERTEX struct
    VERTEX OurVertices[] =
    {
        {
			0.0f, 0.5f*F, 0.0f,
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 0.0f,
			D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f),
			0.0f, 0.0f,
			0
		},
        {
			0.45f*F, -0.5f*F, 0.0f,
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 0.0f,
			D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f),
			0.0f, 0.0f,
			1
		},
        {
			-0.45f*F, -0.5f*F, 0.5f*F,
			0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 0.0f,
			D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f),
			0.0f, 0.0f,
			2
		}
    };


    // create the vertex buffer
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));

    bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
    bd.ByteWidth = sizeof(VERTEX) * 3;             // size is the VERTEX struct * 3
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer

    dev->CreateBuffer(&bd, NULL, &pVBuffer);       // create the buffer


    // copy the vertices into the buffer
    D3D11_MAPPED_SUBRESOURCE ms;
    devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);    // map the buffer
    memcpy(ms.pData, OurVertices, sizeof(OurVertices));                 // copy the data
    devcon->Unmap(pVBuffer, NULL);                                      // unmap the buffer


	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
	bd.ByteWidth = sizeof(CBViewProjection);         
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;       // use as a constant buffer
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer
	
	pvsConstData = new CBViewProjection();
	//memcpy(&pvsConstData->fViewProj.m , ProjectionMatrix(0.0f,100000000.0f,M_PI/2,M_PI/2),sizeof(float)*16);
	memcpy(&pvsConstData->fViewProj.m, new float[16]{1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1}, sizeof(float) * 16);
	//memcpy(&pvsConstData->fViewProj.m, new float[16]{ 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 }, sizeof(float) * 16);
	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = pvsConstData;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	HRESULT hres = dev->CreateBuffer(&bd, &initData, &pCBuffer);       // create the buffer
	if (FAILED(hres)) {
		char error[100];
		sprintf_s(error, "FAIL#2: %d [%08x]", GetLastError(), hres);
		MessageBoxA(0, error, error, 0);
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, hres, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		MessageBoxA(0, messageBuffer, messageBuffer, 0);

		//Free the buffer.
		LocalFree(messageBuffer);

	}
	devcon->VSSetConstantBuffers(0, 1, &pCBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC bdt;
	ZeroMemory(&bdt, sizeof(bdt));
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
	bd.ByteWidth = sizeof(GInstanceBuffer);
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;       // use as a shader resource
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;

	pgInstanceBuffer = new GInstanceBuffer();
	//memcpy(&pvsConstData->fViewProj.m , ProjectionMatrix(0.0f,100000000.0f,M_PI/2,M_PI/2),sizeof(float)*16);
	memcpy(&pgInstanceBuffer->wmat.f, new float[12]{ 1,0,0,0 ,0,1,0,0 ,0,0,1,0 }, sizeof(float) * 16);
	//memcpy(&pvsConstData->fViewProj.m, new float[16]{ 0,0,0,0 ,0,0,0,0 ,0,0,0,0 ,0,0,0,0 }, sizeof(float) * 16);
	// Fill in the subresource data.

	initData.pSysMem = pgInstanceBuffer;
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;
	hres = dev->CreateBuffer(&bd, &initData, &pCBuffer);       // create the buffer
	if (FAILED(hres)) {
		char error[100];
		sprintf_s(error, "FAIL#4: %d [%08x]", GetLastError(), hres);
		MessageBoxA(0, error, error, 0);
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, hres, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		MessageBoxA(0, messageBuffer, messageBuffer, 0);

		//Free the buffer.
		LocalFree(messageBuffer);

	}
	bdt.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	bdt.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
	//bdt.Buffer.NumElements = 1;
	//bdt.Buffer.FirstElement = 0;
	bdt.Buffer.ElementWidth = 4;
	bdt.Buffer.ElementOffset = 0;
	ID3D11ShaderResourceView* pTexture2d;
	hres = dev->CreateShaderResourceView(pCBuffer, &bdt,&pTexture2d);       // create the buffer
	if (FAILED(hres)) {
		char error[100];
		sprintf_s(error, "FAIL#3: %d [%08x]", GetLastError(), hres);
		MessageBoxA(0, error, error, 0);
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, hres, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		MessageBoxA(0, messageBuffer, messageBuffer, 0);

		//Free the buffer.
		LocalFree(messageBuffer);

	}
	devcon->VSSetShaderResources(0, 1, &pTexture2d);
}

ID3D10Blob* loadfile(const char* filePath) {
	ID3D10Blob* buf;
	FILE* f;
	fopen_s(&f,filePath, "rb");
	fseek(f, 0, SEEK_END);
	DWORD fsize = ftell(f);
	D3DCreateBlob(fsize,&buf);
	fseek(f, 0, SEEK_SET);  /* same as rewind(f); */
	fread(buf->GetBufferPointer(), fsize, 1, f);
	fclose(f);
	return buf;
}

// this function loads and prepares the shaders
void InitPipeline()
{
    // load and compile the two shaders
    ID3D10Blob *VS, *PS;
    D3DX11CompileFromFile(L"d:\\tmp\\shaders\\src\\tools\\shaders.shader", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &PS, 0, 0);
	int num;
	LPWSTR* argvw = CommandLineToArgvW(GetCommandLineW(), &num);

	if(num>1){
		char path[256];
		sprintf_s(path, "%ws", argvw[1]);
		VS = loadfile((const char*)&path);
	}else {
		D3DX11CompileFromFile(L"d:\\tmp\\shaders\\src\\tools\\shaders.shader", 0, 0, "VShader", "vs_5_0", 0, 0, 0, &VS, 0, 0);
		//VS = loadfile("d:\\tmp\\shaders\\601_mod2.shdr");
	}
    // encapsulate both shaders into shader objects
	HRESULT hres = dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);
	if (FAILED(hres)) {
		char error[100];
		sprintf_s(error, "FAIL#1: %d [%08x]", GetLastError(), hres);
		MessageBoxA(0, error, error, 0);

		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, hres, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		MessageBoxA(0, messageBuffer, messageBuffer, 0);

		//Free the buffer.
		LocalFree(messageBuffer);

	}
    dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

    // set the shader objects
    devcon->VSSetShader(pVS, 0, 0);
    devcon->PSSetShader(pPS, 0, 0);

    // create the input layout object
    D3D11_INPUT_ELEMENT_DESC ied[] =
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, F3, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, F3 * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UV_Primary", 0, DXGI_FORMAT_R32G32_FLOAT, 0, F3 * 2 + F4, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UV_Secondary", 0, DXGI_FORMAT_R32G32_FLOAT, 0, F3 * 2 + F4 + F2, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, F3 * 2 + F4 + F2 * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"PositionPF", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, F3 * 2 + F4 * 2 + F2 * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"SV_InstanceID", 0, DXGI_FORMAT_R32_UINT, 0, F3 * 3 + F4 * 2 + F2 * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    dev->CreateInputLayout(ied, 8, VS->GetBufferPointer(), VS->GetBufferSize(), &pLayout);
    devcon->IASetInputLayout(pLayout);
}