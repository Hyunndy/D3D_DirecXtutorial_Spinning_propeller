// include the basic windows header files and the Direct3D header file
#include <windows.h>
#include <windowsx.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <fstream>

// define the screen resolution and keyboard macros
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define KEY_DOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 1 : 0)
#define KEY_UP(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000) ? 0 : 1)

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// global declarations
LPDIRECT3D9 d3d;    // the pointer to our Direct3D interface
LPDIRECT3DDEVICE9 d3ddev;    // the pointer to the device class
LPDIRECT3DSURFACE9 z_buffer = NULL;    // the pointer to the z-buffer

std::ofstream OutFile;
									   // mesh declarations
LPD3DXMESH meshSpaceship;    // define the mesh pointer
D3DMATERIAL9* material;    // define the material object
LPDIRECT3DTEXTURE9* texture;    // a pointer to a texture
DWORD numMaterials;    // stores the number of materials in the mesh


					   // function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);    // closes Direct3D and releases memory
void init_graphics(void);    // 3D declarations
void init_light(void);    // sets up the light and the material

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
	wc.lpfnWndProc = (WNDPROC)WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = L"WindowClass1";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL,
		L"WindowClass1",
		L"Our Direct3D Program",
		WS_EX_TOPMOST | WS_POPUP,
		200, 200,
		SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ShowWindow(hWnd, nCmdShow);

	// set up and initialize Direct3D
	initD3D(hWnd);

	// enter the main loop:

	MSG msg;

	while (TRUE)
	{
		DWORD starting_point = GetTickCount();

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		render_frame();

		// check the 'escape' key
		if (KEY_DOWN(VK_ESCAPE))
			PostMessage(hWnd, WM_DESTROY, 0, 0);

		while ((GetTickCount() - starting_point) < 25);
	}

	// clean up DirectX and COM
	cleanD3D();

	return msg.wParam;
}


// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	} break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}


// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;
	d3dpp.EnableAutoDepthStencil = TRUE;    // automatically run the z-buffer for us
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;    // 16-bit pixel format for the z-buffer

												  // create a device class using this information and the info from the d3dpp stuct
	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	// create the z-buffer
	d3ddev->CreateDepthStencilSurface(SCREEN_WIDTH,
		SCREEN_HEIGHT,
		D3DFMT_D16,
		D3DMULTISAMPLE_NONE,
		0,
		TRUE,
		&z_buffer,
		NULL);

	init_graphics();    // call the function to initialize the triangle
	init_light();    // call the function to initialize the light and material

	d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);    // turn on the 3D lighting
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
	d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(200, 200, 200));    // ambient light

	return;
}


// outfile.txt를 통해 메쉬의 기하 정보를 출력하기 위한 Vertex 구조체
struct Vertex
{
	Vertex() {}
	Vertex(float x, float y, float z,
		float nx, float ny, float nz, float u, float v)
	{
		_x = x;   _y = y;   _z = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u = u;   _v = v;
	}

	float _x, _y, _z, _nx, _ny, _nz, _u, _v;

	static const DWORD FVF;
};

// 메쉬의 vertex 정보를 outFile로 출력해내는 함수
void dumpVertices(std::ofstream& outFile, ID3DXMesh* mesh)
{
	outFile << "Vertices:" << std::endl;
	outFile << "---------" << std::endl << std::endl;

	Vertex* v = 0;
	mesh->LockVertexBuffer(0, (void**)&v);
	for (int i = 0; i < mesh->GetNumVertices(); i++)
	{
		outFile << "Vertex " << i << ": (";
		outFile << v[i]._x << ", " << v[i]._y << ", " << v[i]._z << ", ";
		outFile << v[i]._nx << ", " << v[i]._ny << ", " << v[i]._nz << ", ";
		outFile << v[i]._u << ", " << v[i]._v << ")" << std::endl;
	}
	mesh->UnlockVertexBuffer();

	outFile << std::endl << std::endl;
}

const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;
// this is the function used to render a single frame
void render_frame(void)
{
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	//mesh Dum.txt를 연다
	OutFile.open("Mesh Dump.txt");

	dumpVertices(OutFile, meshSpaceship);


	OutFile.close();


	d3ddev->BeginScene();

	// SET UP THE TRANSFORMS

	D3DXMATRIX matView;    // the view transform matrix
	D3DXMatrixLookAtLH(&matView,
		&D3DXVECTOR3(0.0f, 0.0f, -16.0f),    // the camera position
		&D3DXVECTOR3(0.0f, 0.0f, 0.0f),    // the look-at position
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));    // the up direction
	d3ddev->SetTransform(D3DTS_VIEW, &matView);    // set the view transform to matView 

	D3DXMATRIX matProjection;    // the projection transform matrix
	D3DXMatrixPerspectiveFovLH(&matProjection,
		D3DXToRadian(45),    // the horizontal field of view
		SCREEN_WIDTH / SCREEN_HEIGHT,    // the aspect ratio
		1.0f,    // the near view-plane
		100.0f);    // the far view-plane
	d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection);    // set the projection

	//////////////////////////////////////////////////////////////////////
	// 프로펠러 회전과 비행기 전체 회전
	//////////////////////////////////////////////////////////////////////

	static float index = 0.0f;
	index += 0.3f;
	if (index > D3DXToRadian(360)) index = 0.0f;

	// Y축을 기준으로 돌게하는 MATRIX
	D3DXMATRIX matRotateY;    
	D3DXMatrixRotationY(&matRotateY, index);    

	// 프로펠러만 돌게하는 MATRIX
	D3DXMATRIX trans1, rot, trans2, Final;

	// 프로펠러 = 서브셋[5]을 이루는 메쉬의 X,Y,Z좌표를 3으로 나눈것.
	// 왜 0으로 하는지 모르겠다.. ㅎ
	float vx = 0;
	float vy = (-0.400603 + 0.254447 + 0.503541)/3;
	float vz =  (-2.16404 - 0.762626 - 0.762633)/3;

	D3DXVECTOR3 axis(vx, vy, vz);
	//axis벡터를 정규화해준다. 정규화란 데이터의 범위를 일치시키거나 분포를 유사하게 만들어준다.
	D3DXVec3Normalize(&axis, &axis); 
	// 사용자가 정해준 축(Axis)기준으로 도는 함수.
	D3DXMatrixRotationAxis(&rot, &axis, index);
	// 프로펠러를 원점으로 이동시켜준다.
	D3DXMatrixTranslation(&trans1, -vx, -vy, -vz);
	// 프로펠러를 다시 원위치로 복구시켜준다.
	D3DXMatrixTranslation(&trans2, vx, vy, vz);


// draw the spaceship
	for (DWORD i = 0; i < numMaterials; i++)    // loop through each subset
	{
	
		if (i == 5)

			// Final 벡터를 최종 벡터로 해준다.
			// 1. 프로펠러를 원점으로 이동시켜준다.
			// 2. 사용자가 정한 축(axis) = 프로펠러의 center을 기준으로 돈다.
			// 3. 다시 원위치로 복귀한다.
			Final = trans2 * rot * trans1;

		else
			// 서브셋[5]일때만 Final행렬이 필요하므로 이용이 끝나면 Final을 단위벡터로 만들어준다.
			D3DXMatrixIdentity(&Final); 
			// 단위행렬xmatRotateY = 프로펠러가 아닌 비행기 구성품은 matRotateY만하게 한다.
			d3ddev->SetTransform(D3DTS_WORLD, &(Final*matRotateY));
		
		d3ddev->SetMaterial(&material[i]);    // set the material for the subset
		if (texture[i] != NULL)    // if the subset has a texture (if texture is not NULL)
			d3ddev->SetTexture(0, texture[i]);    // ...then set the texture
		meshSpaceship->DrawSubset(i);    // draw the subset
	}

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);

	return;
}


// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	meshSpaceship->Release();    // close and release the spaceship mesh
	d3ddev->Release();    // close and release the 3D device
	d3d->Release();    // close and release Direct3D

	return;
}


// this is the function that puts the 3D models into video RAM
void init_graphics(void)
{
	LPD3DXBUFFER bufShipMaterial;

	D3DXLoadMeshFromX(L"airplane 2.x",    // load this file
		D3DXMESH_SYSTEMMEM,    // load the mesh into system memory
		d3ddev,    // the Direct3D Device
		NULL,    // we aren't using adjacency
		&bufShipMaterial,    // put the materials here
		NULL,    // we aren't using effect instances
		&numMaterials,    // the number of materials in this model
		&meshSpaceship);    // put the mesh here

							// retrieve the pointer to the buffer containing the material information
	D3DXMATERIAL* tempMaterials = (D3DXMATERIAL*)bufShipMaterial->GetBufferPointer();

	// create a new material buffer and texture for each material in the mesh
	material = new D3DMATERIAL9[numMaterials];
	texture = new LPDIRECT3DTEXTURE9[numMaterials];

	for (DWORD i = 0; i < numMaterials; i++)    // for each material...
	{
		material[i] = tempMaterials[i].MatD3D;    // get the material info
		material[i].Ambient = material[i].Diffuse;    // make ambient the same as diffuse
													  // if there is a texture to load, load it
		if (FAILED(D3DXCreateTextureFromFileA(d3ddev,
			tempMaterials[i].pTextureFilename,
			&texture[i])))
			texture[i] = NULL;    // if there is no texture, set the texture to NULL
	}

	return;
}


// this is the function that sets up the lights and materials
void init_light(void)
{
	D3DLIGHT9 light;    // create the light struct

	ZeroMemory(&light, sizeof(light));    // clear out the struct for use
	light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
	light.Diffuse.r = 0.5f;    // .5 red
	light.Diffuse.g = 0.5f;    // .5 green
	light.Diffuse.b = 0.5f;    // .5 blue
	light.Diffuse.a = 1.0f;    // full alpha (we'll get to that soon)

	D3DVECTOR vecDirection = { -1.0f, -0.3f, -1.0f };    // the direction of the light
	light.Direction = vecDirection;    // set the direction

	d3ddev->SetLight(0, &light);    // send the light struct properties to light #0
	d3ddev->LightEnable(0, TRUE);    // turn on light #0

	return;
}
