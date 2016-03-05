// dxHelloworld1.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "dxHelloworld1.h"
#include <d3d11.h>
#include <DirectXColors.h>
#include "cameraclass.h"
#include "modelclass.h"
#include "colorshaderclass.h"
#include <DirectXMath.h>
#include "openvr.h"
#include "rendertextureclass.h"
#include "debugwindowclass.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "openvr_api.lib")

#define MAX_LOADSTRING 100
//#define VR_DISABLED

// ȫ�ֱ���: 
HINSTANCE hInst;                                // ��ǰʵ��
WCHAR szTitle[MAX_LOADSTRING];                  // �������ı�
WCHAR szWindowClass[MAX_LOADSTRING];            // ����������
UINT clientWidth = 1280;
UINT clientHeight = 720;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.1f;
const float MOVE_STEP = 0.3f;
const float ROTATE_STEP = 5;
D3DXMATRIX m_projectionMatrix;

D3DXMATRIX m_orthoMatrix;

// d3d global declarations
ID3D11Device*			pDevice	= nullptr;
ID3D11DeviceContext*	pImmediateContext = nullptr;
ID3D11Texture2D*	pBackBufferTex;
IDXGISwapChain*			pSwapChain = nullptr;
ID3D11RenderTargetView*	pRenderTargetView = nullptr;
ID3D11DepthStencilView* pDepthStencilView = nullptr;
ID3D11DepthStencilState * pDSState;
ID3D11DepthStencilState* m_depthDisabledStencilState;
D3D_DRIVER_TYPE			driverType;
D3D_FEATURE_LEVEL		featureLevel;
D3D11_VIEWPORT			viewport;
CameraClass* m_CameraLeft = nullptr,
			*m_CameraRight = nullptr;
ModelClass* m_Model = nullptr;
ColorShaderClass* m_ColorShader = nullptr;
RenderTextureClass* m_RenderTextureLeft, *m_RenderTextureRight;
DebugWindowClass* m_DebugWindowLeft, *m_DebugWindowRight;


uint32_t m_nRenderWidth;
uint32_t m_nRenderHeight;

float m_fNearClip;
float m_fFarClip;

Matrix4 m_mat4HMDPose;
Matrix4 m_mat4eyePosLeft;
Matrix4 m_mat4eyePosRight;

Matrix4 m_mat4ProjectionCenter;
Matrix4 m_mat4ProjectionLeft;
Matrix4 m_mat4ProjectionRight;

vr::IVRSystem *m_pHMD;
vr::IVRRenderModels *m_pRenderModels;
vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];

int m_iTrackedControllerCount;
int m_iTrackedControllerCount_Last;
int m_iValidPoseCount;
int m_iValidPoseCount_Last;
bool m_bShowCubes;

std::string m_strPoseClasses;                            // what classes we saw poses for this frame
char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class



namespace Memory
{
	template <class T> void SafeDelete(T& t)
	{
		if (t)
		{
			delete t;
			t = nullptr;
		}
	}

	template <class T> void SafeDeleteArr(T& t)
	{
		if (t)
		{
			delete[]t;
			t = nullptr;
		}
	}

	template <class T> void SafeRelease(T& t)
	{
		if (t)
		{
			t->Release();
			t = nullptr;
		}
	}
}

// �˴���ģ���а����ĺ�����ǰ������: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// d3d function prototypes
bool init(HWND hWnd);
void render_frame(void);
void clean(void);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: �ڴ˷��ô��롣

    // ��ʼ��ȫ���ַ���
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DXHELLOWORLD1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // ִ��Ӧ�ó����ʼ��: 
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DXHELLOWORLD1));

    MSG msg;
	bool done, result;


	// Initialize the message structure.
	ZeroMemory(&msg, sizeof(MSG));

	// Loop until there is a quit message from the window or the user.
	done = false;
	while (!done)
	{
		// Handle the windows messages.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// If windows signals to end the application then exit out.
		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			// Otherwise do the frame processing.
			render_frame();
		}

	}

	clean();

    return (int) msg.wParam;
}



void MyDebug(LPCWSTR msg)
{
	MessageBox(NULL, msg, _T(""), 0);
}

//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DXHELLOWORLD1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DXHELLOWORLD1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��: 
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, clientWidth, clientHeight, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   init(hWnd);

   return TRUE;
}

void Move(float x, float y, float z = 0)
{
	Vector3 pos;
	pos = m_CameraLeft->GetPosition();
	pos.x += x * MOVE_STEP;
	pos.y += y * MOVE_STEP;
	pos.z += z * MOVE_STEP;

	m_CameraLeft->SetPosition(pos.x, pos.y, pos.z);

	pos = m_CameraRight->GetPosition();
	pos.x += x * MOVE_STEP;
	pos.y += y * MOVE_STEP;
	pos.z += z * MOVE_STEP;

	m_CameraRight->SetPosition(pos.x, pos.y, pos.z);
}

void Rotate(float x, float y)
{
	Vector3 rot;
	rot= m_CameraLeft->GetRotation();
	rot.x += x*ROTATE_STEP;
	rot.y += y*ROTATE_STEP;

	m_CameraLeft->SetRotation(rot.x, rot.y, rot.z);

	rot= m_CameraRight->GetRotation();
	rot.x += x*ROTATE_STEP;
	rot.y += y*ROTATE_STEP;

	m_CameraRight->SetRotation(rot.x, rot.y, rot.z);
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��:    ���������ڵ���Ϣ��
//
//  WM_COMMAND  - ����Ӧ�ó���˵�
//  WM_PAINT    - ����������
//  WM_DESTROY  - �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // �����˵�ѡ��: 
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_LEFT:
					Move(-1, 0);
					break;
				case VK_RIGHT:
					Move(1, 0);
					break;
				case VK_UP:
					Move(0, 1);
					break;
				case VK_DOWN:
					Move(0, -1);
					break;
				case 'W':
					Move(0, 0, 1);
					break;
				case 'S':
					Move(0, 0, -1);
					break;
				case 'Q':
					Rotate(1, 0);
					break;
				case 'E':
					Rotate(-1, 0);
					break;
				case 'A':
					Rotate(0, -1);
					break;
				case 'D':
					Rotate(0, 1);
					break;
			}
		}
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: �ڴ˴����ʹ�� hdc ���κλ�ͼ����...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);

	return matrixObj.invert();
}

Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip, vr::API_OpenGL);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
}

void SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}

#ifndef  VR_DISABLED



bool SetupStereoRenderTargets()
{
	if (!m_pHMD)
		return false;

	//m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	//CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	return true;
}

#endif // ! VR_DISABLED

void dprintf(const char *fmt, ...)
{
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);


	OutputDebugStringA(buffer);
}
Matrix4 ConvertSteamVRMatrixToMatrix4( const vr::HmdMatrix34_t &matPose )
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
	return matrixObj;
}

void UpdateHMDMatrixPose()
{
	if (!m_pHMD)
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_Other:             m_rDevClassChar[nDevice] = 'O'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd].invert();
	}
}

// this function initializes D3D and VR
bool init(HWND hWnd)
{
	UINT createDeviceFlags = 0;

#ifdef DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#ifndef VR_DISABLED

	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;

	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, ARRAYSIZE(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		string temp(buf);
		wstring wtemp(temp.begin(), temp.end());
		MessageBox(hWnd, wtemp.c_str(), L"VR_Init Failed", 0);
		return false;
	}

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	dprintf("width = %d, height = %d", m_nRenderWidth, m_nRenderHeight);

	m_nRenderWidth /= 2;
	m_nRenderHeight /= 4;


	m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
	if (!m_pRenderModels)
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();

		char buf[1024];
		sprintf_s(buf, ARRAYSIZE(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		string temp(buf);
		wstring wtemp(temp.begin(), temp.end());
		MessageBox(hWnd, wtemp.c_str(), L"VR_Init Failed", NULL);
		return false;
	}

	if (!vr::VRCompositor())
	{
		dprintf("Compositor initialization failed. See log file for details\n");
		return false;
	}

#endif

	// CREATE DEVICE AND SWAP CHAIN
	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, // the first thing to try, if failed, go to the next
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0, // texture size and others..
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC swapDesc;
	ZeroMemory(&swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	swapDesc.BufferCount = 1;
	swapDesc.BufferDesc.Width = clientWidth;
	swapDesc.BufferDesc.Height = clientHeight;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // unsigned normal
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = hWnd;
	swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapDesc.Windowed = true;
	swapDesc.SampleDesc.Count = 1; // multisampling, which antialiasing for geometry. Turn it off
	swapDesc.SampleDesc.Quality = 0;
	swapDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // alt-enter fullscreen
	
	swapDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	HRESULT errorCode;
	for (unsigned i = 0; i < numDriverTypes; ++i)
	{
		errorCode = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[i], NULL, createDeviceFlags,
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &swapDesc, &pSwapChain, &pDevice,
			&featureLevel, &pImmediateContext);

		if (SUCCEEDED(errorCode))
		{
			driverType = driverTypes[i];
			break;
		}
	}

	if (FAILED(errorCode))
	{
		OutputDebugString(_T("FAILED TO CREATE DEVICE AND SWAP CHAIN"));
		MyDebug(_T("FAILED TO CREATE DEVICE AND SWAP CHAIN"));
		return false;
	}

	HRESULT result;
	// CREATE RENDER TARGET VIEW
	result = pSwapChain->GetBuffer(NULL, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBufferTex));
	if (FAILED(result))
	{
		return false;
	}

	D3D11_TEXTURE2D_DESC BBDesc;
	ZeroMemory(&BBDesc, sizeof(D3D11_TEXTURE2D_DESC));
	pBackBufferTex->GetDesc(&BBDesc);

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	ZeroMemory(&RTVDesc, sizeof(D3D11_RENDER_TARGET_VIEW_DESC));
	RTVDesc.Format = BBDesc.Format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	//RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	RTVDesc.Texture2D.MipSlice = 0;

	result = pDevice->CreateRenderTargetView(pBackBufferTex, &RTVDesc, &pRenderTargetView);
	if (FAILED(result))
	{
		MyDebug(_T("ERROR"));
	}
	//Memory::SafeRelease(pBackBufferTex);

	// CREATE DEPTH STENCIL
	ID3D11Texture2D* pDepthStencil = NULL;
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = swapDesc.BufferDesc.Width;
	descDepth.Height = swapDesc.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;// DXGI_FORMAT_D32_FLOAT;//DXGI_FORMAT_D24_UNORM_S8_UINT;;//pDeviceSettings->d3d11.AutoDepthStencilFormat;
	// DXGI_FORMAT_D32_FLOAT_S8X24_UINT
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	result = pDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil);
	if (FAILED(result))
		return false;


	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));
	// Depth test parameters
	dsDesc.DepthEnable = true;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = true;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create depth stencil state
	result = pDevice->CreateDepthStencilState(&dsDesc, &pDSState);
	if (FAILED(result))
	{
		return false;
	}

	// Bind depth stencil state
	pImmediateContext->OMSetDepthStencilState(pDSState, 1);



	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;// DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	//descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	descDSV.Texture2D.MipSlice = 0;

	// Create the depth stencil view
	result = pDevice->CreateDepthStencilView(pDepthStencil, // Depth stencil texture
		&descDSV, // Depth stencil desc
		&pDepthStencilView);  // [out] Depth stencil view

	if (FAILED(result))
	{
		WCHAR buf[100];
		wsprintf(buf, L"%x", result);
		MyDebug(buf);
		MyDebug(L"CreateDepthStencilView failed.");
		return false;
	}

	//			 // Bind the depth stencil view
	//pImmediateContext->OMSetRenderTargets(1,          // One rendertarget view
	//	&pRTV,      // Render target view, created earlier
	//	pDSV);     // Depth stencil view for the render target

				   //BIND RENDER TARGET VIEW
	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView); // depth stencil view is for shadow map
	

	D3D11_DEPTH_STENCIL_DESC depthDisabledStencilDesc;
																					 // Clear the second depth stencil state before setting the parameters.
	ZeroMemory(&depthDisabledStencilDesc, sizeof(depthDisabledStencilDesc));

	// Now create a second depth stencil state which turns off the Z buffer for 2D rendering.  The only difference is 
	// that DepthEnable is set to false, all other parameters are the same as the other depth stencil state.
	depthDisabledStencilDesc.DepthEnable = false;
	depthDisabledStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthDisabledStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthDisabledStencilDesc.StencilEnable = true;
	depthDisabledStencilDesc.StencilReadMask = 0xFF;
	depthDisabledStencilDesc.StencilWriteMask = 0xFF;
	depthDisabledStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthDisabledStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	depthDisabledStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthDisabledStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthDisabledStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the state using the device.
	result = pDevice->CreateDepthStencilState(&depthDisabledStencilDesc, &m_depthDisabledStencilState);
	if (FAILED(result))
	{
		return false;
	}

	//VIEWPORT CREATION
	viewport.Width = static_cast<float>(clientWidth);
	viewport.Height = static_cast<float>(clientHeight);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// BIND VIEWPORT
	pImmediateContext->RSSetViewports(1, &viewport);




	// Create the camera object.
	m_CameraLeft = new CameraClass;
	if (!m_CameraLeft)
	{
		return false;
	}

	// Set the initial position of the camera.
	m_CameraLeft->SetPosition(0.0f, 0.0f, -10.0f);

	// Create the camera object.
	m_CameraRight = new CameraClass;
	if (!m_CameraRight)
	{
		return false;
	}

	// Set the initial position of the camera.
	m_CameraRight->SetPosition(1.5f, 0.0f, -10.0f);

	// Create the model object.
	m_Model = new ModelClass;
	if (!m_Model)
	{
		return false;
	}

	// Initialize the model object.
	result = m_Model->Initialize(pDevice, L"cube_texture.png");
	if (!result)
	{
		MessageBox(hWnd, L"Could not initialize the model object.", L"Error", MB_OK);
		return false;
	}

	// Create the color shader object.
	m_ColorShader = new ColorShaderClass;
	if (!m_ColorShader)
	{
		return false;
	}

	// Initialize the color shader object.
	result = m_ColorShader->Initialize(pDevice, hWnd);
	if (!result)
	{
		MessageBox(hWnd, L"Could not initialize the color shader object.", L"Error", MB_OK);
		return false;
	}


	// Create the render to texture object.
	m_RenderTextureLeft = new RenderTextureClass;
	if (!m_RenderTextureLeft)
	{
		return false;
	}

	// Initialize the render to texture object.
	result = m_RenderTextureLeft->Initialize(pDevice, m_nRenderWidth, m_nRenderHeight);
	if (!result)
	{
		return false;
	}

	m_RenderTextureRight = new RenderTextureClass;
	if (!m_RenderTextureRight)
	{
		return false;
	}

	// Initialize the render to texture object.
	result = m_RenderTextureRight->Initialize(pDevice, m_nRenderWidth, m_nRenderHeight);
	if (!result)
	{
		return false;
	}

	// Create the debug window object.
	m_DebugWindowLeft = new DebugWindowClass;
	if (!m_DebugWindowLeft)
	{
		return false;
	}

	// Initialize the debug window object.
	result = m_DebugWindowLeft->Initialize(pDevice, clientWidth, clientHeight, clientWidth/2, clientHeight);
	if (!result)
	{
		MessageBox(hWnd, L"Could not initialize the debug window object.", L"Error", MB_OK);
		return false;
	}

	m_DebugWindowRight = new DebugWindowClass;
	if (!m_DebugWindowRight)
	{
		return false;
	}
	// Initialize the debug window object.
	result = m_DebugWindowRight->Initialize(pDevice, clientWidth, clientHeight, clientWidth/2, clientHeight);

	// Setup the projection matrix.
	float fieldOfView = (float)3.14159265359 / 4.0f;
	float screenAspect = (float)clientWidth / (float)clientHeight;

	// Create the projection matrix for 3D rendering.
	//D3DXMatrixPerspectiveFovLH(&m_projectionMatrix, fieldOfView, screenAspect, SCREEN_NEAR, SCREEN_DEPTH);
	DirectX::XMMATRIX m = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, SCREEN_NEAR, SCREEN_DEPTH);
	m_projectionMatrix.set((const float*)&m.r);


	// Create an orthographic projection matrix for 2D rendering.
	//D3DXMatrixOrthoLH(&m_orthoMatrix, (float)screenWidth, (float)screenHeight, screenNear, screenDepth);
	DirectX::XMMATRIX mo = DirectX::XMMatrixOrthographicLH((float)m_nRenderWidth, (float)m_nRenderHeight, SCREEN_NEAR, SCREEN_DEPTH);
	m_orthoMatrix.set((const float*)&mo.r);

	SetupCameras();

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}

	return true;

	//d3d = Direct3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface

	//D3DPRESENT_PARAMETERS d3dpp;    // create a struct to hold various device information

	//ZeroMemory(&d3dpp, sizeof(d3dpp));    // clear out the struct for use
	//d3dpp.Windowed = TRUE;    // program windowed, not fullscreen
	//d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	//d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D


	//							   // create a device class using this information and the info from the d3dpp stuct
	//d3d->CreateDevice(D3DADAPTER_DEFAULT,
	//	D3DDEVTYPE_HAL,
	//	hWnd,
	//	D3DCREATE_SOFTWARE_VERTEXPROCESSING,
	//	&d3dpp,
	//	&d3ddev);
}


void TurnZBufferOn()
{
	pImmediateContext->OMSetDepthStencilState(pDSState, 1);
	return;
}


void TurnZBufferOff()
{
	pImmediateContext->OMSetDepthStencilState(m_depthDisabledStencilState, 1);
	return;
}


bool errorshown = false;
unsigned frame_count = 0;

bool RenderScene(CameraClass *camera)
{
	bool result;
	D3DXMATRIX viewMatrix, projectionMatrix, worldMatrix, orthoMatrix;

	//m_Camera->SetPosition(0, 0, -10);

	// Generate the view matrix based on the camera's position.
	camera->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	camera->GetViewMatrix(viewMatrix);
	//m_D3D->GetWorldMatrix(worldMatrix);
	worldMatrix.identity();
	//m_D3D->GetProjectionMatrix(projectionMatrix);
	projectionMatrix = m_projectionMatrix;

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_Model->Render(pImmediateContext);

	//// test code
	//viewMatrix.identity();
	//projectionMatrix.identity();
	//worldMatrix.identity();
	// Render the model using the color shader.
	result = m_ColorShader->Render(pImmediateContext, m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture());
	if (!errorshown && !result)
	{
		errorshown = true;
		return false;
		MyDebug(_T("render failed"));
	}

	return true;
}

bool RenderToTexture()
{
	bool result;


	// Set the render target to be the render to texture.
	m_RenderTextureLeft->SetRenderTarget(pImmediateContext, pDepthStencilView);
	//Clear the render to texture background to blue so we can differentiate it from the rest of the normal scene.

		// Clear the render to texture.
		m_RenderTextureLeft->ClearRenderTarget(pImmediateContext, pDepthStencilView, 0.0f, 0.0f, 1.0f, 1.0f);

	// Render the scene now and it will draw to the render to texture instead of the back buffer.
	result = RenderScene(m_CameraLeft);
	if (!result)
	{
		return false;
	}

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	//pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

	// Set the render target to be the render to texture.
	m_RenderTextureRight->SetRenderTarget(pImmediateContext, pDepthStencilView);
	//Clear the render to texture background to blue so we can differentiate it from the rest of the normal scene.

		// Clear the render to texture.
		m_RenderTextureRight->ClearRenderTarget(pImmediateContext, pDepthStencilView, 0.0f, 0.0f, 1.0f, 1.0f);

	// Render the scene now and it will draw to the render to texture instead of the back buffer.
	result = RenderScene(m_CameraRight);
	if (!result)
	{
		return false;
	}

	// Reset the render target back to the original back buffer and not the render to texture anymore.
	pImmediateContext->OMSetRenderTargets(1, &pRenderTargetView, pDepthStencilView);

	return true;
}

// this is the function used to render a single frame
void render_frame(void)
{

	bool result;

	// Render the entire scene to the texture first.
	result = RenderToTexture();
	if (!result)
	{
		return;
	}

	pImmediateContext->ClearRenderTargetView(pRenderTargetView, DirectX::Colors::CornflowerBlue);
	pImmediateContext->ClearDepthStencilView(pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

		D3DXMATRIX viewMatrix, projectionMatrix, worldMatrix, orthoMatrix;

	//m_Camera->SetPosition(0, 0, -10);

	// Generate the view matrix based on the camera's position.
	m_CameraLeft->Render();

	// Get the world, view, and projection matrices from the camera and d3d objects.
	m_CameraLeft->GetViewMatrix(viewMatrix);
	//m_D3D->GetWorldMatrix(worldMatrix);
	worldMatrix.identity();
	//m_D3D->GetProjectionMatrix(projectionMatrix);
	projectionMatrix = m_projectionMatrix;

	// Put the model vertex and index buffers on the graphics pipeline to prepare them for drawing.
	m_Model->Render(pImmediateContext);

	//// test code
	//viewMatrix.identity();
	//projectionMatrix.identity();
	//worldMatrix.identity();
	// Render the model using the color shader.
	result = m_ColorShader->Render(pImmediateContext, m_Model->GetIndexCount(), worldMatrix, viewMatrix, projectionMatrix, m_Model->GetTexture());
	if(!errorshown && !result)
	{
		errorshown = true;
		//return false;
		MyDebug(_T("render failed"));
	}


	TurnZBufferOff();

	result = m_DebugWindowLeft->Render(pImmediateContext, 50, 50);
	if (!result)
	{
		return;
	}

	orthoMatrix = m_orthoMatrix;

	// Render the debug window using the texture shader.
	result = m_ColorShader->Render(pImmediateContext, m_DebugWindowLeft->GetIndexCount(), worldMatrix, viewMatrix,
		orthoMatrix, m_RenderTextureLeft->GetShaderResourceView());
	if (!result)
	{
		return;
	}


	result = m_DebugWindowRight->Render(pImmediateContext, 640, 50);
	if (!result)
	{
		return;
	}

	orthoMatrix = m_orthoMatrix;

	// Render the debug window using the texture shader.
	result = m_ColorShader->Render(pImmediateContext, m_DebugWindowRight->GetIndexCount(), worldMatrix, viewMatrix,
		orthoMatrix, m_RenderTextureRight->GetShaderResourceView());
	if (!result)
	{
		return;
	}

	TurnZBufferOn();


	pSwapChain->Present(0, 0);


	vr::Texture_t leftEyeTexture = { m_RenderTextureLeft->GetTexture(), vr::API_DirectX, vr::ColorSpace_Gamma };
	vr::EVRCompositorError error1 =  vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
	vr::Texture_t rightEyeTexture = { m_RenderTextureRight->GetTexture(), vr::API_DirectX, vr::ColorSpace_Auto};
	vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	if (error1)
		dprintf("error is %d \n", error1);

	UpdateHMDMatrixPose();
	//// clear the window to a deep blue
	//d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);

	//d3ddev->BeginScene();    // begins the 3D scene

	//						 // do 3D rendering on the back buffer here

	//d3ddev->EndScene();    // ends the 3D scene

	//d3ddev->Present(NULL, NULL, NULL, NULL);   // displays the created frame on the screen
	frame_count++;
}


// this is the function that cleans up D3D and VR
void clean(void)
{
		// Release the color shader object.
	if(m_ColorShader)
	{
		m_ColorShader->Shutdown();
		delete m_ColorShader;
		m_ColorShader = 0;
	}

	// Release the model object.	
	if(m_Model)
	{
		m_Model->Shutdown();
		delete m_Model;
		m_Model = 0;
	}

	// Release the camera object.
	if(m_CameraLeft)
	{
		delete m_CameraLeft;
		m_CameraLeft = 0;
	}

	if (pImmediateContext) pImmediateContext->ClearState();
	Memory::SafeRelease(pDepthStencilView);
	Memory::SafeRelease(pRenderTargetView);
	Memory::SafeRelease(pSwapChain);
	Memory::SafeRelease(pImmediateContext);
	Memory::SafeRelease(pDevice);


	//d3ddev->Release();    // close and release the 3D device
	//d3d->Release();    // close and release Direct3D
}