#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<cmath>
#include<algorithm>
#include<vector>
#include<queue>
#include<cstring>
#include<string>
#include<string.h>
#include<fstream>
#include<istream>
#include<ostream>
#include<iomanip>
#include<Windows.h>
#include<conio.h>
#include<tchar.h>
#include<atlconv.h> 
using namespace std;
#define BUF_SIZE 8192
char NameUser[] = "USER";
char NameIn[50] = "INPUT";
char NameOut[50] = "OUTPUT";
char NameIoo[50] = "INPUTOROUTPUT";

class User {
public:
	char name[50];
	
	User() {
		memset(name, '\0', sizeof(name));
	}
};

class ShareMemory {
public:
	int cnt;
	char str[20][300];

	ShareMemory() {
		cnt = 0;
		memset(str, '\0', sizeof(str));
	}
	void clear() {
		cnt = 0;
		memset(str, '\0', sizeof(str));
	}
}smo;

class InputOrOutput {
public:
	int toshell;//simdisk给shell的通知
	int tosimdisk;//shell给simdisk的通知
	InputOrOutput() {
		toshell = 0;
		tosimdisk = 0;
	}
};

HANDLE hMapFileUser;
User user;
User* pBufUser = &user;

HANDLE hMapFileIoo;
InputOrOutput* pBufIoo = NULL;

HANDLE hMapFileIn;
ShareMemory stin;
ShareMemory* pBufIn = &stin;

HANDLE hMapFileOut;
ShareMemory* pBufOut = NULL;

void GetUser() {//获得用户名，用于与simdisk建立连接
	//创建共享文件
	hMapFileUser = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE,
		NameUser);
	if (hMapFileUser == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
	pBufUser = (User*)MapViewOfFile(hMapFileUser,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufUser == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileUser);
		return;
	}

	//清空
	char tmpc[50]; memset(tmpc, '\0', sizeof(tmpc));
	tmpc[0] = 'N'; tmpc[1] = 'O'; tmpc[2] = 'N'; tmpc[3] = 'E';
	for (int i = 0; i < 50; i++)
		pBufUser->name[i] = tmpc[i];

	//输入用户名
	cout << "Please input the username: ";
	cin >> pBufUser->name;

	//获得NameIn、NameOut、NameIoo的值
	strcat_s(NameIn, pBufUser->name);
	strcat_s(NameOut, pBufUser->name);
	strcat_s(NameIoo, pBufUser->name);

	//等待simdisk读取用户名
	
	while (true) {
		string tmps = string(pBufUser->name);
		if (tmps == "GET") {//simdisk已经获得了用户名
			for (int i = 0; i < 50; i++)
				pBufUser->name[i] = tmpc[i];
			break;
		}
	}

	//关闭
	UnmapViewOfFile(pBufUser);
	CloseHandle(hMapFileUser);

	Sleep(10);
	return;
}

void Input() {//向输入共享内存存放一行数据，flag==true时有效
	//创建共享文件
	hMapFileIn = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE,
		NameIn);
	if (hMapFileIn == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
	pBufIn = (ShareMemory*)MapViewOfFile(hMapFileIn,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufIn == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileIn);
		return;
	}

	//清空缓冲区
	pBufIn->cnt = 0;
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 300; j++) {
			pBufIn->str[i][j] = 0;
		}
	}
	
	//获取内容，写入共享内存
	while (cin >> pBufIn->str[pBufIn->cnt]) {
		pBufIn->cnt++;
		if (cin.get() == '\n') break;
	}

	//通知simdisk端要输入的数据已全部放入共享内存
	pBufIoo->tosimdisk = 1;

	//等simdisk端全部读取
	while (true) {
		if (pBufIoo->toshell == 0) {//simdisk端通知已读入全部数据
			pBufIoo->tosimdisk = 0;
			break;
		}
	}

	//关闭
	UnmapViewOfFile(pBufIn);
	CloseHandle(hMapFileIn);

	Sleep(10);
	return;
}
void Output() {//读取共享内存中的数据，并输出
	// 打开命名共享内存
	hMapFileOut = OpenFileMapping(
		FILE_MAP_READ,
		FALSE,
		NameOut);
	if (hMapFileOut == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，获取里面的数据
	pBufOut = (ShareMemory*)MapViewOfFile(hMapFileOut,
		FILE_MAP_READ,
		0,
		0,
		BUF_SIZE);
	if (pBufOut == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileOut);
		return;
	}

	//清空对象
	smo.clear();

	//获取
	smo.cnt = pBufOut->cnt;
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 300; j++) {
			smo.str[i][j] = pBufOut->str[i][j];
		}
	}
	for (int i = 0; i < smo.cnt; i++) {
		cout << smo.str[i];
	}
	
	pBufIoo->tosimdisk = 2;//通知simdisk端数据已全部输出
	while (true) {
		if (pBufIoo->toshell == 0) {//simdisk端通知shell端知道已经输出
			pBufIoo->tosimdisk = 0;
			break;
		}
	}

	//关闭
	UnmapViewOfFile(pBufOut);
	CloseHandle(hMapFileOut);

	Sleep(10);
	return;
}

void Run() {
	// 打开命名共享内存
	hMapFileIoo = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		NameIoo);
	if (hMapFileIoo == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，获取里面的数据
	pBufIoo = (InputOrOutput*)MapViewOfFile(hMapFileIoo,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufIoo == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileIoo);
		return;
	}

	//获取
	while (true) {
		if (pBufIoo->toshell == 0) {

		}
		else if (pBufIoo->toshell == 1) {
			Input();
		}
		else if (pBufIoo->toshell == 2) {
			Output();
		}
	}

	//卸载内存映射文件地址指针
	UnmapViewOfFile(pBufIoo);
	//关闭内存映射文件
	CloseHandle(hMapFileIoo);

	return;
}

int main() {
	//与simdisk建立连接
	GetUser();

	//必须要等待一段时间，等simdisk端建立Ioo的共享内存区
	Sleep(100);

	//运行
	Run();

	return 0;
}