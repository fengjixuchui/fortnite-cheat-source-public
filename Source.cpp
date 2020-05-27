#include "DirectOverlay.h"
#include <D3dx9math.h>
#include <sstream>
#include <string>
#include <algorithm>
#include <list>

#define M_PI 3.14159265358979323846264338327950288419716939937510

#define OFFSET_UWORLD 0x8345058

bool Menu = false;
bool Aimbot = true;
bool EnemyESP = true;
bool skeleton = false;
bool BoxESP = true;
bool LineESP = false;
bool DistanceESP = true;
bool DrawFov = false;

DWORD processID;
HWND hwnd = NULL;
 
int width;
int height;
int localplayerID;
float FovAngle;

HANDLE DriverHandle;
uint64_t base_address;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;

Vector3 localactorpos;
Vector3 Localcam;

float AimFOV = 120.f; 

bool isaimbotting;
DWORD_PTR entityx;

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x420);

	return read<FTransform>(DriverHandle, processID, bonearray + (index * 0x30));
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(DriverHandle, processID, mesh + 0x1C0);

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DXMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}

Vector3 ProjectWorldToScreen(Vector3 WorldLocation)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);
	Vector3 Camera;

	auto Nigger = read<uintptr_t>(DriverHandle, processID, Localplayer + 0xa8);
	uint64_t Nigger1 = read<uintptr_t>(DriverHandle, processID, Nigger + 8);

	Camera.x = read<float>(DriverHandle, processID, Nigger1 + 0x678);
	Camera.y = read<float>(DriverHandle, processID, Rootcomp + 0x12C);

	float test = asin(Camera.x);
	float degrees = test * (180.0 / M_PI);
	Camera.x = degrees;

	if (Camera.y < 0)
		Camera.y = 360 + Camera.y;

	D3DMATRIX tempMatrix = Matrix(Camera);
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	/*world->owningGameInstance->playersArray->localPlayer + 0x70] + 0x98] + 0x130] + 0x10
	world->owningGameInstance->playersArray->localPlayer + 0x20] + 0xAB0] + 0x130] + 0x10
	world->persistentLevel + 0x20] + 0x178] + 0x130] + 0x10
	world->persistentLevel + 0xB8] + 0x178] + 0x130] + 0x10*/

	uint64_t chain = read<uint64_t>(DriverHandle, processID, Ulevel + 0x20);
	uint64_t chain1 = read<uint64_t>(DriverHandle, processID, chain + 0x178);
	uint64_t chain2 = read<uint64_t>(DriverHandle, processID, chain1 + 0x130);

	Vector3 vDelta = WorldLocation - read<Vector3>(DriverHandle, processID, chain2 + 0x10); //camera location credits for Object9999
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float zoom = read<float>(DriverHandle, processID, Nigger1 + 0x500);

	float FovAngle = 80.0f / (zoom / 1.19f);
	float ScreenCenterX = width / 2.0f;
	float ScreenCenterY = height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;

	return Screenlocation;
}

void menu()
{
	if (Menu)
	{
		DrawBox(5, 10, 150, 140, 0.0f, 0.f, 0.f, 0.f, 100, true);
        DrawString(_xor_("Fucking noobs").c_str(), 17, 10, 8, 255.f, 255.f, 255.f, 255.f);

		if (EnemyESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 110, 10 + 20, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 110, 10 + 20, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F1] Enemy ESP >>").c_str(), 13, 10, 10 + 20, 255.f, 255.f, 255.f, 255.f);

		if (BoxESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 93, 10 + 40, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 93, 10 + 40, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F2] Box ESP >>").c_str(), 13, 10, 10 + 40, 255.f, 255.f, 255.f, 255.f);

		if (LineESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 95, 10 + 60, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 95, 10 + 60, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F3] Line ESP >>").c_str(), 13, 10, 10 + 60, 255.f, 255.f, 255.f, 255.f);

		if (DistanceESP)
			DrawString(_xor_("ON").c_str(), 13, 10 + 120, 10 + 80, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 120, 10 + 80, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F4] Distance ESP >>").c_str(), 13, 10, 10 + 80, 255.f, 255.f, 255.f, 255.f);

		if (skeleton)
			DrawString(_xor_("ON").c_str(), 13, 10 + 120, 10 + 100, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 120, 10 + 100, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F5] Skeleton ESP >>").c_str(), 13, 10, 10 + 100, 255.f, 255.f, 255.f, 255.f);

		if (Aimbot)
			DrawString(_xor_("ON").c_str(), 13, 10 + 90, 10 + 120, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("OFF").c_str(), 13, 10 + 90, 10 + 120, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F6] Aimbot >>").c_str(), 13, 10, 10 + 120, 255.f, 255.f, 255.f, 255.f);
	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		HWND test = FindWindowA(0, _xor_("Fortnite  ").c_str());

		if (test == NULL)
		{
			ExitProcess(0);
		}

		if (GetAsyncKeyState(VK_INSERT) & 1) {
			Menu = !Menu;
		}

		if (Menu)
		{
			if (GetAsyncKeyState(VK_F1) & 1) {
				EnemyESP = !EnemyESP;
			}

			if (GetAsyncKeyState(VK_F2) & 1) {
				BoxESP = !BoxESP;
			}

			if (GetAsyncKeyState(VK_F3) & 1) {
				LineESP = !LineESP;
			}

			if (GetAsyncKeyState(VK_F4) & 1) {
				DistanceESP = !DistanceESP;
			}

			if (GetAsyncKeyState(VK_F5) & 1) {
				skeleton = !skeleton;
			}

			if (GetAsyncKeyState(VK_F6) & 1) {
				Aimbot = !Aimbot;
			}
		}
	}
}

void aimbot(float x, float y)
{
	float ScreenCenterX = (width / 2);
	float ScreenCenterY = (height / 2);
	int AimSpeed = 1.0f;
	float TargetX = 0;
	float TargetY = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);

	return;
}

void AimAt(DWORD_PTR entity)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x278);
	auto rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead);

	if (rootHeadOut.y != 0 || rootHeadOut.y != 0)
	{
		aimbot(rootHeadOut.x, rootHeadOut.y);
	}
}

void DrawSkeleton(DWORD_PTR mesh)
{
	Vector3 vHeadBoneOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 96));
	Vector3 vHipOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 2));
	Vector3 vNeckOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 65));
	Vector3 vUpperArmLeftOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 34));
	Vector3 vUpperArmRightOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 91));
	Vector3 vLeftHandOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 35));
	Vector3 vRightHandOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 63));
	Vector3 vLeftHandOut1 = ProjectWorldToScreen(GetBoneWithRotation(mesh, 33));
	Vector3 vRightHandOut1 = ProjectWorldToScreen(GetBoneWithRotation(mesh, 60));
	Vector3 vRightThighOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 74));
	Vector3 vLeftThighOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 67));
	Vector3 vRightCalfOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 75));
	Vector3 vLeftCalfOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 68));
	Vector3 vLeftFootOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 69));
	Vector3 vRightFootOut = ProjectWorldToScreen(GetBoneWithRotation(mesh, 76));

	DrawLine(vHipOut.x, vHipOut.y, vNeckOut.x, vNeckOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vUpperArmLeftOut.x, vUpperArmLeftOut.y, vNeckOut.x, vNeckOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vUpperArmRightOut.x, vUpperArmRightOut.y, vNeckOut.x, vNeckOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vUpperArmLeftOut.x, vUpperArmLeftOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vUpperArmRightOut.x, vUpperArmRightOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vLeftHandOut1.x, vLeftHandOut1.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vRightHandOut1.x, vRightHandOut1.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftThighOut.x, vLeftThighOut.y, vHipOut.x, vHipOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vRightThighOut.x, vRightThighOut.y, vHipOut.x, vHipOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftCalfOut.x, vLeftCalfOut.y, vLeftThighOut.x, vLeftThighOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vRightCalfOut.x, vRightCalfOut.y, vRightThighOut.x, vRightThighOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftFootOut.x, vLeftFootOut.y, vLeftCalfOut.x, vLeftCalfOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
	DrawLine(vRightFootOut.x, vRightFootOut.y, vRightCalfOut.x, vRightCalfOut.y, 2.f, 255.f, 0.f, 0.f, 200.f);
}

void drawLoop(int width, int height) {
	menu();

	DrawCircle(width / 2, height / 2, AimFOV, 2.f, 255.f, 0.f, 0.f, 255.f, false);

	Uworld = read<DWORD_PTR>(DriverHandle, processID, base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x188);

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(DriverHandle, processID, Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(DriverHandle, processID, LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(DriverHandle, processID, Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(DriverHandle, processID, PlayerController + 0x298);

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(DriverHandle, processID, LocalPawn + 0x130);

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(DriverHandle, processID, LocalPawn + 0x18);
	}

	Ulevel = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD64 PlayerState = read<DWORD64>(DriverHandle, processID, LocalPawn + 0x238);

	if (PlayerState == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(DriverHandle, processID, Ulevel + 0xA0);

	DWORD_PTR AActors = read<DWORD_PTR>(DriverHandle, processID, Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	float closestDistance = FLT_MAX;
	DWORD_PTR closestPawn = NULL;

	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = read<uint64_t>(DriverHandle, processID, AActors + i * 0x8);

		int curactorid = read<int>(DriverHandle, processID, CurrentActor + 0x18);

		if (curactorid == localplayerID || curactorid == localplayerID + 121)
		{
			if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
				continue;

			uint64_t CurrentActorRootComponent = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x130);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x278);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			int MyTeamId = read<int>(DriverHandle, processID, PlayerState + 0xE60);

			DWORD64 otherPlayerState = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x238);

			if (otherPlayerState == (uint64_t)nullptr || otherPlayerState == -1 || otherPlayerState == NULL)
				continue;

			int ActorTeamId = read<int>(DriverHandle, processID, otherPlayerState + 0xE60);

			if (MyTeamId != ActorTeamId)
			{
				Vector3 Headpos = GetBoneWithRotation(currentactormesh, 66);
				localactorpos = read<Vector3>(DriverHandle, processID, Rootcomp + 0x11C);

				float distance = localactorpos.Distance(Headpos) / 100.f;

				//W2S
				Vector3 HeadposW2s = ProjectWorldToScreen(Headpos);
				Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);
				Vector3 bottom = ProjectWorldToScreen(bone0);
				Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15));
				Vector3 Aimpos = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 10));

				float Height1 = abs(Headbox.y - bottom.y);
				float Width1 = Height1 * 0.65;

				if (skeleton)
					DrawSkeleton(currentactormesh);

				if (BoxESP)
					DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 2.f, 255.f, 0.f, 0.f, 200.f, false);

				if (EnemyESP)
					DrawString(_xor_("Enemy").c_str(), 13, HeadposW2s.x, HeadposW2s.y - 25, 0, 1, 1);

				if (DistanceESP)
				{
					CHAR dist[50];
					sprintf_s(dist, _xor_("[%.f]").c_str(), distance);
					DrawString(dist, 13, HeadposW2s.x + 40, HeadposW2s.y - 25, 0, 1, 1);
				}

				if (LineESP)
					DrawLine(width / 2, height, bottom.x, bottom.y, 2.f, 255.f, 0.f, 0.f, 200.f);

				if (Aimbot)
				{
					auto dx = HeadposW2s.x - (width / 2);
					auto dy = HeadposW2s.y - (height / 2);
					auto dist = sqrtf(dx * dx + dy * dy);

					if (dist < AimFOV && dist < closestDistance) {
						closestDistance = dist;
						closestPawn = CurrentActor;
					}
				}
			}
		}
	}

	if (Aimbot)
	{
		if (closestPawn && GetAsyncKeyState(0x05) < 0) { //change aimkey here
			AimAt(closestPawn);
		}
	}
}

void main()
{
	DriverHandle = CreateFileW(_xor_(L"\\\\.\\nbcv53nc").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (DriverHandle == INVALID_HANDLE_VALUE)
	{
		printf(_xor_("Load Driver first, exiting...\n").c_str());
		Sleep(2000);
		exit(0);
	}

	info_t Input_Output_Data1;
	unsigned long int Readed_Bytes_Amount1;
	DeviceIoControl(DriverHandle, ctl_clear, &Input_Output_Data1, sizeof Input_Output_Data1, &Input_Output_Data1, sizeof Input_Output_Data1, &Readed_Bytes_Amount1, nullptr);

	while (hwnd == NULL)
	{
		hwnd = FindWindowA(0, _xor_("Fortnite  ").c_str());

		printf(_xor_("Looking for process...\n").c_str());
		Sleep(10);
	}
	GetWindowThreadProcessId(hwnd, &processID);

	RECT rect;
	if(GetWindowRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	info_t Input_Output_Data;
	Input_Output_Data.pid = processID;
	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(DriverHandle, ctl_base, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
	base_address = (unsigned long long int)Input_Output_Data.data;
	std::printf(_xor_("Process base address: %p.\n").c_str(), (void*)base_address);

	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);
	
	DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_COURIER);
	DirectOverlaySetup(drawLoop, hwnd);
	getchar();
}
