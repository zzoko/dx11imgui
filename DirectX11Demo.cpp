#include "includes.h"




namespace Process {
	DWORD ID;
	HANDLE Handle;
	HWND Hwnd;
	HMODULE Module;
	WNDPROC oWndProc; //modified with "o"
	int WindowWidth;
	int WindowHeight;
	LPCSTR Title;
	LPCSTR ClassName;
	LPCSTR Path;
}

namespace DirectX11Interface {
	ID3D11Device* Device;
	ID3D11DeviceContext* DeviceContext;
	ID3D11RenderTargetView* RenderTargetView;
}



//---------------------------------------------------------------------------------------------------- END OF INCLUDES


typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef HRESULT(__stdcall* ResizeBuffers)(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

// Setup for the Proxy
EXTERN_C IMAGE_DOS_HEADER __ImageBase; // We might need this some day. Don't remove.
HMODULE Module = 0; // Declare our "extern HMODULE ourModule" from proxy.cpp inside dllmain, so we can pass hModule to Proxy_Attach();

//extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);



Present oPresent;
ResizeBuffers oResizeBuffers;
HWND Hwnd = NULL;
WNDPROC oWndProc; //error
ID3D11Device* Device = NULL;
ID3D11DeviceContext* DeviceContext = NULL;
ID3D11RenderTargetView* RenderTargetView;



bool init = false;
bool init_hook = false;

//Function Defs
// 
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT __stdcall MJPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
HRESULT hkResizeBuffers(IDXGISwapChain* pThis, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
DWORD WINAPI MainThread(HMODULE hmod);
void Init_Hooks();


//---------------------------------------------------------------------------------------------------- END OF Function Defs



//dx11 ResizeBuffers Hook
HRESULT hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{




	if (RenderTargetView) {
		DeviceContext->OMSetRenderTargets(0, 0, 0);
		RenderTargetView->Release();
	}

	HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer);
	// Perform error handling here!

	Device->CreateRenderTargetView(pBuffer, NULL, &RenderTargetView);
	// Perform error handling here!
	pBuffer->Release();

	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = Width;
	vp.Height = Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	DeviceContext->RSSetViewports(1, &vp);
	return hr;
}

//---------------------------------------------------------------------------------------------------- END OF Function Defs

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (show_menu) {
		ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
		return true;
	}
	return CallWindowProc(Process::oWndProc, hwnd, uMsg, wParam, lParam);
}



HRESULT APIENTRY MJPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
	if (!ImGui_Initialised) {
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&DirectX11Interface::Device))){
			ImGui::CreateContext();

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantTextInput || ImGui::GetIO().WantCaptureKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

			DirectX11Interface::Device->GetImmediateContext(&DirectX11Interface::DeviceContext);

			DXGI_SWAP_CHAIN_DESC Desc;
			pSwapChain->GetDesc(&Desc);
			WindowHwnd = Desc.OutputWindow;

			ID3D11Texture2D* BackBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&BackBuffer);
			DirectX11Interface::Device->CreateRenderTargetView(BackBuffer, NULL, &DirectX11Interface::RenderTargetView);
			BackBuffer->Release();

			ImGui_ImplWin32_Init(WindowHwnd);
			ImGui_ImplDX11_Init(DirectX11Interface::Device, DirectX11Interface::DeviceContext);
			ImGui_ImplDX11_CreateDeviceObjects();
			ImGui::GetIO().ImeWindowHandle = Process::Hwnd;
			Process::oWndProc = (WNDPROC)SetWindowLongPtr(Process::Hwnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
			ImGui_Initialised = true;
		}
	}

	if (GetAsyncKeyState(VK_F1) & 1) show_menu = !show_menu;
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	ImGui::GetIO().MouseDrawCursor = show_menu;

	if (show_menu == true) 
	{

	//	ImGui::ShowDemoWindow();

		CONTROLS::DISABLE_ALL_CONTROL_ACTIONS(0); //disable game input with menu open


		    // first time menu size
		if (imgui_first_time)
		{
			ImGui::SetNextWindowPos(ImVec2(434, 271));
			ImGui::SetNextWindowSize(ImVec2(611, 506));
			ImGui::SetNextWindowCollapsed(false, ImGuiCond_FirstUseEver);
			imgui_first_time = false;
		}

		ImGui::Begin("Menu");
		ImGui::Checkbox("Enable", &menu_enabled);

		if (menu_enabled)
		{
			ImGui::SameLine();
			info();
			ImGui::NewLine();


			if (ImGui::CollapsingHeader("Time & Weather"))
			{
				// time control

				draw_time_menu();



				// weather control

				draw_weather_menu();

				ImGui::NewLine();
			}
			
			if (ImGui::CollapsingHeader("Wanted level"))
			{
				// wanted level control

				ImGui::NewLine();
				ImGui::Checkbox("Lock Wanted Level", &lock_wanted_level);
				ImGui::SliderInt("Wanted Level", &wanted_level, 0, 5, "Wanted Level: %d");
				if (ImGui::IsItemActive() || ImGui::IsItemClicked())
				{
					set_wanted_level();
				}
				ImGui::NewLine();
			}



			if (ImGui::CollapsingHeader("Vehicles"))
			{
				// vehicle control

				RenderSearchBar();

				ImGui::SameLine();

				repair();

				ImGui::NewLine();
			}


			if (ImGui::CollapsingHeader("Weapons"))
			{
				// gun control

				draw_weapon_menu();
				prepare_weapon();

				ImGui::NewLine();
			}

			if (ImGui::CollapsingHeader("Player"))
			{
				// god mode control

				ImGui::NewLine();
				ImGui::Checkbox("God Mode", &god_mode);


				if (god_mode)
				{
					set_god_mode();
				}


				if (prev_god_mode && !god_mode)
				{
					no_god_mode();
				}

				prev_god_mode = god_mode;




				// clean ped

				ImGui::SameLine();
				ImGui::Checkbox("Clean Ped", &clean_ped);
				


				// waypoint teleport

				teleport();

				ImGui::SameLine();
				tp_menu_hotkey();



				ImGui::NewLine();
			}





			//------------------
			//test





		








			//-------------------










		}


		else
		{
			// if menu disabled, reset all options

			clear_override_weather();
			strncpy_s(current_weather, "", sizeof(current_weather));


			lock_time = false;


			lock_wanted_level = false;


			god_mode = false;
			no_god_mode();


			spawn_inside =true;


			clean_ped = false;











		}


		ImGui::End();







	}
	ImGui::EndFrame();
	ImGui::Render();
	DirectX11Interface::DeviceContext->OMSetRenderTargets(1, &DirectX11Interface::RenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


	//return oIDXGISwapChainPresent(pSwapChain, SyncInterval, Flags);

	//new for reisizing window
	return oPresent(pSwapChain, SyncInterval, Flags);

}

void APIENTRY MJDrawIndexed(ID3D11DeviceContext* pContext, UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation) 
{
}

DWORD WINAPI MainThread(LPVOID lpParameter) {
	bool WindowFocus = false;
	while (WindowFocus == false) {
		DWORD ForegroundWindowProcessID;
		GetWindowThreadProcessId(GetForegroundWindow(), &ForegroundWindowProcessID);
		if (GetCurrentProcessId() == ForegroundWindowProcessID) {

			Process::ID = GetCurrentProcessId();
			Process::Handle = GetCurrentProcess();
			Process::Hwnd = GetForegroundWindow();

			RECT TempRect;
			GetWindowRect(Process::Hwnd, &TempRect);
			Process::WindowWidth = TempRect.right - TempRect.left;
			Process::WindowHeight = TempRect.bottom - TempRect.top;

			char TempTitle[MAX_PATH];
			GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle));
			Process::Title = TempTitle;

			char TempClassName[MAX_PATH];
			GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName));
			Process::ClassName = TempClassName;

			char TempPath[MAX_PATH];
			GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath));
			Process::Path = TempPath;

			WindowFocus = true;
		}
	}
	bool InitHook = false;
	while (InitHook == false) {
		if (DirectX11::Init() == true) {
		    CreateHook(8, (void**)&oIDXGISwapChainPresent, MJPresent);
			CreateHook(12, (void**)&oID3D11DrawIndexed, MJDrawIndexed);


			//hook 13 for window resize test
			CreateHook(13, (void**)&oResizeBuffers, hkResizeBuffers);

			InitHook = true;
		}
	}
	return 0;
}




bool isInitialized = false; //temporary


void main()
{

	while (true)
	{

		//loops every frame

		if (!isInitialized) 
		{
			InitializeVehicleModels();  //load cars list
			isInitialized = true;
		}


		if (menu_enabled)
		{


			if (lock_time)
			{
				always_set_time();
			}




			if (lock_wanted_level)
			{
				set_wanted_level();
			}




			if (spawned)
			{
				SpawnVehicle();

				spawned = false;
			}




			if (clean_ped)
			{
				always_clean_ped();
			}



			


			start_tp_hotkey();








		}


		WAIT(0);

	}

}




void ScriptMain()
{
	srand(GetTickCount64());
	main();
}










BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) 
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		if (ChecktDirectXVersion(DirectXVersion.D3D11) == true) {
			Process::Module = hModule;
			scriptRegister(hModule, ScriptMain);
			keyboardHandlerRegister(OnKeyboardMessage);
			CreateThread(0, 0, MainThread, 0, 0, 0);
		}
		break;
	case DLL_PROCESS_DETACH:
		scriptUnregister(hModule);
		keyboardHandlerUnregister(OnKeyboardMessage);
		FreeLibraryAndExitThread(hModule, TRUE);
		DisableAll();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}

