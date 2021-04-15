#include <windows.h>
#include <stdio.h>
#include <psapi.h>
#include <string>
#include <time.h>
#include <math.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <future>


#define PI 3.14159265

#define WINDOWNAME "Counter-Strike Source"
#define PLRSZ 0x10
#define SERVERDLL "server.dll"
#define CLIENTDLL "hl2.exe"
#define ENGINEDLL "engine.dll"


struct v3
{
	float x = 0;
	float y = 0;
	float z = 0;
	int hp1 = 0;
	int team_f = 0;		//2 - t,  3 - ct
	float distant = 0;
	int num = 1;
};

struct angle2
{
	float a_x = 0;
	float a_y = 0;
};


using namespace std;



//52AA3FEC - struct1
DWORD engine_dll_base = 0;

const DWORD angle_x = 0x5D8691B8;	 //engine.dll+4791B8
const DWORD angle_y = 0x5D8691B4;	 //engine.dll+4791B4

const DWORD plr_num_offset = 0x4EFFE0;
const DWORD plr_list_offset = 0x004F3FEC;
const DWORD hp_offset = 0xE4;
const DWORD coords_offset = 0x308;
const DWORD angle_offset = 0xA18;
const DWORD team_offset = 0x1F4;
const DWORD ctrl_offset = 0x408;

DWORD addr;
DWORD plr_addr3 = 0;
DWORD server_dll_base;
DWORD client_dll_base;
HANDLE hProcess;
HWND hWnd;
float my_coords[3];
float ang[2];
HDC hDC;

int team = 0;
bool k = 0;
angle2 temp_result = {0,0};


angle2 me_a = { ang[1],ang[0] };
v3 me = { my_coords[0],my_coords[1],my_coords[2],100,team,0.0 };

v3  enem[100] = { 0,0,0,0,team,0.0 };
v3 my_coords_temp;
v3 enem_coords_temp;

int tr = 1;
int en = 1;
int flag = 0;
int players_on_map = 0;
int max_count_ppl = 0;
angle2 result = { 10, 10 };
angle2 answ = { 0,0 };

float dista(v3, v3);
void get_process_handle();
int read_bytes(PVOID addr, int num, void* buf);
void esp();
void aim(angle2, v3, v3);


int main(int argc, char** argv)
{

	auto ftr = std::async(std::launch::async, []() {


		get_process_handle();

		while (1)
		{

			esp();

			max_count_ppl = 0;
			DWORD max_count_ppl_addr = engine_dll_base + 0x5E96BC;
			read_bytes((PVOID)(max_count_ppl_addr), 4, &max_count_ppl);

			DWORD plr_addr2;
			int hp2;

			tr = 1;
			int k = 1;
			int t = 1;
			for (int i = 1; i < max_count_ppl; i++)
			{
				int ctrl = 0;

				read_bytes((PVOID)(addr + i * PLRSZ), 4, &plr_addr2);
				read_bytes((PVOID)(plr_addr2 + hp_offset), 4, &hp2);
				read_bytes((PVOID)(plr_addr2 + team_offset), 4, &team);



				enem[i].hp1 = hp2;
				enem[i].team_f = team;
				enem[i].num = i;
				enem[i].distant = dista(me, enem[i]);
				cout << "hp2_" << i << "ogo = " << hp2 << " dist= " << enem[i].distant << endl;

			}

			for (int i = 1; i < max_count_ppl; i++)
			{
				for (int j = 1; j < max_count_ppl; j++)
				{
					if (enem[i].distant < enem[j].distant)
					{
						v3 temp = { 0,0,0,0,0,0,0 };

						temp = enem[j];
						enem[j] = enem[i];
						enem[i] = temp;
						if (enem[j].hp1 == 1)
						{
							enem[j] = temp;
							enem[j].hp1 = 1;
						}
					}
				}
			}

			tr = 1;

			for (int i = 1; i < max_count_ppl; i++)
			{
				if (((enem[i].hp1 == 0 || enem[i].hp1 == 1) && tr == i))
				{
					if (enem[i].hp1 == 1)
					{
						tr++;
					}

					if (tr == max_count_ppl - 1)
					{
						tr = 1;
						break;
					}


				}

				while (me.team_f == enem[tr].team_f)
				{
					tr++;
				}

			}


			aim(me_a, me, enem[tr]);
			cout << "tr = " << tr << endl;


		}


		CloseHandle(hProcess);


	});

	return 0;
}


void read_my_coords(DWORD addr)
{
	int my_hp = 0;
	int my_team = 0;
	DWORD plr_addr;
	read_bytes((PVOID)(addr), 4, &plr_addr);
	read_bytes((PVOID)(plr_addr + coords_offset), 12, &my_coords);
	read_bytes((PVOID)(plr_addr + hp_offset), 4, &my_hp);
	read_bytes((PVOID)(plr_addr + team_offset), 4, &my_team);
	cout << "my_coords = " << my_coords[0] << ":" << my_coords[1] << ":" << my_coords[2] << endl;
	me.x = my_coords[0]; 
	me.y = my_coords[1];
	me.z = my_coords[2];
	me.hp1 = my_hp;
	me.team_f = my_team;
	me.num = 1;
	//{ my_coords[0], my_coords[1], my_coords[2], my_hp, my_team, 1 };
	
}

void read_my_angle(DWORD addr)
{
	DWORD plr_addr;
	read_bytes((PVOID)(addr), 4, &plr_addr);
	read_bytes((PVOID)(plr_addr + angle_offset), 8, &ang);
	cout << "my_angle x= " << ang[1] << "  y= " << ang[0] << endl;
	me_a = { ang[1],ang[0] };
}

void esp()
{
	int ctrl = 0;
	v3 c = { 0,0,0 };

	int players_on_map, i, hp;
	float coords[3];
	DWORD plr_addr;
	addr = server_dll_base + plr_list_offset;
	read_bytes((PVOID)addr, 4, &plr_addr);
	read_bytes((PVOID)(server_dll_base + plr_num_offset), 4,
		&players_on_map);
	printf("players on the map: %d\n", players_on_map);
	printf("players on the screeen:\n");
	read_my_coords(addr);
	read_my_angle(addr);

	for (i = 1; i < players_on_map; i++) {
		read_bytes((PVOID)(addr + i * PLRSZ), 4, &plr_addr);
		read_bytes((PVOID)(plr_addr + hp_offset), 4, &hp);
		enem[i].hp1 = hp;

		if (hp == 0 || hp == 1)
		{
			players_on_map++;
		}
		else {
			read_bytes((PVOID)(plr_addr + coords_offset), 12, &coords);
			c.x = coords[0];
			c.y = coords[1];
			c.z = coords[2] - 2;
			c.hp1 = hp;

			read_bytes((PVOID)(plr_addr + ctrl_offset), 4, &ctrl);

			/*if ((ctrl % 2) == 1)
				c.z = c.z - 30;*/


			enem[i] = c;
			float tempCoords[3];
			printf("player %d health: %d (%g:%g:%g)\n", i, hp, coords[0], coords[1], coords[2]);

		}




	}




}


float dista(v3 me, v3 enemy)
{
	float dis = 0;
	dis = sqrt(pow((me.x - enemy.x), 2) + pow((me.y - enemy.y), 2) + pow((me.z - enemy.z), 2));


	return dis;
}


void aim(angle2 me_a, v3 me, v3 enemy)
{


	

		DWORD angle_x_addr_1 = engine_dll_base + 0x4791B8;
		DWORD angle_y_addr_1 = engine_dll_base + 0x4791B4;

		v3 dot = { 0,0,0 };
		v3 dot1 = { 0,0,0 };
		v3 dot_test = { 0,0,0 };
		

		v3 help = { 0,0,0 };
		float xi = 0;
		float dist;


		if ( ( (result.a_x > 0.3) || (result.a_y > 0.3) ) ||
			( (enemy.distant != enem_coords_temp.distant) || (enemy.hp1 != enem_coords_temp.hp1)) )
		{

			my_coords_temp = me;
			enem_coords_temp = enemy;

			dist = dista(me, enem[en]);


			dot.x = me.x + 0.001 + cos(me_a.a_x * PI / 180.0);
			dot.y = me.y + 0.001 + sin(me_a.a_x * PI / 180.0);
			dot.z = me.z;

			dot1.x = me.x;
			dot1.y = me.y;
			dot1.z = me.z + 0.001 + sin(me_a.a_y* PI / 180.0);

			cout << "x dot = " << dot.x << endl;
			cout << "y dot = " << dot.y << endl;
			cout << "z dot = " << dot.z << endl;



			v3 me_enemy = { me.x - enemy.x,me.y - enemy.y,me.z - enemy.z };
			v3 dot_me = { me.x - dot.x,me.y - dot.y,me.z - dot.z };
			v3 dot1_me = { me.x - dot1.x,me.y - dot1.y,me.z - dot1.z };
			cout << "me_enemy = " << me_enemy.x << ":" << me_enemy.y << ":" << me_enemy.z << endl;
			cout << "dot_me = " << dot_me.x << ":" << dot_me.y << ":" << dot_me.z << endl;
			float param_x;
			float param_y;

			param_x = (dot_me.x * me_enemy.x + dot_me.y * me_enemy.y) / (sqrt(dot_me.x*dot_me.x + dot_me.y*dot_me.y)*sqrt(me_enemy.x*me_enemy.x + me_enemy.y*me_enemy.y));
			result.a_x = acos(param_x) * 180.0 / PI;

			param_y = (dot1_me.x * me_enemy.x + dot1_me.y * me_enemy.y + dot1_me.z * me_enemy.z) / (sqrt(dot1_me.x*dot1_me.x + dot1_me.y*dot1_me.y + dot1_me.z*dot1_me.z)*sqrt(me_enemy.x*me_enemy.x + me_enemy.y*me_enemy.y + me_enemy.z*me_enemy.z));

			float c = acos(param_y) * 180.0 / PI;



			result.a_y = c;
			cout << "dist = " << dist << endl;
			cout << "y angle = " << result.a_y << endl;
			cout << "x angle = " << result.a_x << endl;
			cout << "me y angle = " << me_a.a_y << endl;
			cout << "me x angle = " << me_a.a_x << endl;


			if (enemy.z < me.z)
			{
				if (result.a_y > 90)
				{
					answ.a_y = result.a_y - 90;
				}
				else
				{
					answ.a_y = 90 - result.a_y;
				}
			}
			else
			{
				if (result.a_y > 90)
				{
					answ.a_y = 90 - result.a_y;
				}
				else
				{
					answ.a_y = result.a_y - 90;
				}
			}


			cout << "ans x1 angle = " << answ.a_x << endl;
			float temp_result_x = result.a_x;

			answ.a_x = me_a.a_x + result.a_x;

			cout << "ans x2 angle = " << answ.a_x << endl;
			/*dot_test.x = my_coords[0] + 0.001 + (me_enemy.x - 10);// *cos(answ.a_x * PI / 180.0);
			dot_test.y = my_coords[1] + 0.001 + (me_enemy.y - 10);// *sin(answ.a_x * PI / 180.0);
			float terrer = sqrt( pow((dot_test.x - enemy.x), 2) + pow((dot_test.y - enemy.y), 2) );

			cout << "dot_test.x = " << dot_test.x << " : " << dot_test.y << endl;
			cout << "dist test dot - enemy" << terrer;
			cout << "answ.a_x" << answ.a_x << endl;*/


			me_a.a_x = answ.a_x;
			cout << "me x2 angle = " << answ.a_x << endl;

			my_coords_temp = me;
			enem_coords_temp = enemy;

			dist = dista(me, enem[en]);


			dot.x = me.x + 0.001 + cos(me_a.a_x * PI / 180.0);
			dot.y = me.y + 0.001 + sin(me_a.a_x * PI / 180.0);
			dot.z = me.z;

			cout << "x dot = " << dot.x << endl;
			cout << "y dot = " << dot.y << endl;
			cout << "z dot = " << dot.z << endl;



			me_enemy = { me.x - enemy.x,me.y - enemy.y,me.z - enemy.z };
			dot_me = { me.x - dot.x,me.y - dot.y,me.z - dot.z };
			cout << "me_enemy = " << me_enemy.x << ":" << me_enemy.y << ":" << me_enemy.z << endl;
			cout << "dot_me = " << dot_me.x << ":" << dot_me.y << ":" << dot_me.z << endl;
			param_x;
			param_y;

			param_x = (dot_me.x * me_enemy.x + dot_me.y * me_enemy.y) / (sqrt(dot_me.x*dot_me.x + dot_me.y*dot_me.y)*sqrt(me_enemy.x*me_enemy.x + me_enemy.y*me_enemy.y));
			result.a_x = acos(param_x) * 180.0 / PI;

			
			int temp = result.a_x;
			cout << "temp = " << temp << endl;

			if ( temp > 0.3)
			{
				answ.a_x = answ.a_x - 2 * temp_result_x;
			}

			cout << "ans y angle = " << answ.a_y << endl;

			result.a_x = abs(answ.a_x - me_a.a_x);
			result.a_y = abs(answ.a_y - me_a.a_y);
			
		}

	cout << "ans x angle = " << answ.a_x << endl;

	int terr = GetKeyState(VK_LBUTTON) & 0x800;
	cout << "terr1 = " << terr;
	if (GetKeyState(VK_LBUTTON) & 0x800)
	{
		int delay = 100;
		
		
		WriteProcessMemory(hProcess, (LPVOID)angle_x_addr_1, &(answ.a_x), (DWORD)sizeof(answ.a_x), NULL);
		WriteProcessMemory(hProcess, (LPVOID)angle_y_addr_1, &(answ.a_y), (DWORD)sizeof(answ.a_y), NULL);
		

		int k = 0;
		int hp3 = 100;
		
		while ( //(hp3 > 1) && 
			(k < 3))
		{
			Sleep(delay);
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
			k++;
			answ.a_y = answ.a_y + 0.3 + k/5;

			read_bytes((PVOID)(addr + tr * PLRSZ), 4, &plr_addr3);
			read_bytes((PVOID)(plr_addr3 + hp_offset), 4, &hp3);

			WriteProcessMemory(hProcess, (LPVOID)angle_x_addr_1, &(answ.a_x), (DWORD)sizeof(answ.a_x), NULL);
			WriteProcessMemory(hProcess, (LPVOID)angle_y_addr_1, &(answ.a_y), (DWORD)sizeof(answ.a_y), NULL);
		}
			cout << " terr2 = " << terr << endl;
	}
	system("cls");




}



void get_process_handle()
{
	DWORD pid = 0;
	hWnd = FindWindow(0, WINDOWNAME);
	if (hWnd == 0) {
		printf("FindWindow failed, %08X\n", GetLastError());
		return;
	}
	GetWindowThreadProcessId(hWnd, &pid);
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
	if (hProcess == 0) {
		printf("OpenProcess failed, %08X\n", GetLastError());
		return;
	}
	hDC = GetDC(hWnd);
	HMODULE hMods[1024];
	int i;
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &pid) == 0) {
		printf("enumprocessmodules failed, %08X\n", GetLastError());
	}
	else {
		for (i = 0; i < (pid / sizeof(HMODULE)); i++) {
			TCHAR szModName[MAX_PATH];
			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR))) {
				if (strstr(szModName, SERVERDLL) != 0) {
					printf("server.dll base: %08X\n", hMods[i]);
					server_dll_base = (DWORD)hMods[i];
				}
				if (strstr(szModName, CLIENTDLL) != 0) {
					printf("client.dll base: %08X\n", hMods[i]);
					client_dll_base = (DWORD)hMods[i];
				}
				if (strstr(szModName, ENGINEDLL) != 0) {
					printf("engine.dll base: %08X\n", hMods[i]);
					engine_dll_base = (DWORD)hMods[i];
				}
			}
		}
	}
}

int read_bytes(PVOID addr, int num, void* buf)
{
	SIZE_T sz = 0;
	int r = ReadProcessMemory(hProcess, addr, buf, num, &sz);
	if (r == 0 || sz == 0) {
		printf("RPM error, %08X\n", GetLastError());
		return 0;
	}
	return 1;
}



