#ifndef TestData
#define TestData
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
using namespace std;
ifstream in;
ofstream out;

class FileSystemTree {
public:
	string name, path;//文件名和文件路径
	int fath;//上级目录编号
	int type;//文件类型，0==dir，1==file
	int sonfile[200], sonnum;//子文件编号及数量
	int fileindex;//若为二进制文件类型，记录对应文件集的编号

	FileSystemTree() {
		name = ""; path = "";
		fath = -1;
		type = -1;
		memset(sonfile, 0, sizeof(sonfile)); sonnum = 0;
		fileindex = -1;
	}
	void clear() {
		name = ""; path = "";
		fath = -1;
		type = -1;
		memset(sonfile, 0, sizeof(sonfile)); sonnum = 0;
		fileindex = -1;
	}
}fs[50000];
int FileCnt = 0;

bool FindName(int x, string s) {
	for (int i = 0; i < fs[x].sonnum; i++) {
		if (fs[fs[x].sonfile[i]].name == s) return true;
	}
	return false;
}

double random(double l, double r) {
	return (double)rand() / (double)RAND_MAX * (r - l) + l;
}

int roulette(double p_sel[], int num) {
	double val = 0, total = 0;
	for (int i = 0; i < num; i++) total += p_sel[i];
	val = random(0, total);
	total = 0;
	for (int i = 0; i < num; i++) {
		total += p_sel[i];
		if (val < total) return i;
	}
	return num - 1;
}


void RandFile() {//随机生成1000个文件，文件大小从1个字符到15w个字符递增
	string path1 = "D:\\code\\Linux_ext2\\FileSet\\";
	for (int i = 0, len = 1; i < 1000; i++, len += 150) {
		string path2 = path1 + to_string(i) + ".txt";
		out.open(path2, ios::out);
		for (int j = 0; j < len; j++) {
			char c = char((int)'a' + rand() % 26);
			out << c;
		}
		out.close();
	}

	return;
}

string RandFileName() {//随机8位文件名
	string s;
	for (int i = 0; i < 8; i++) {
		s += char((int)'A' + rand() % 26);
	}
	return s;
}

void RandMd() {//随机10000个新增目录的命令
	//初始化根目录
	fs[0].name = "/"; fs[0].path = "";
	fs[0].type = 0;
	FileCnt++;
	//随机输出
	out.open("orders.txt", ios::out | ios::app);
	for (int i = 1; i < 10001; i++) {
		//随机选择一个可以添加新目录的上级目录
		int dir = rand() % FileCnt;
		while (fs[dir].sonnum == 200) {
			dir = rand() % FileCnt;
		}
		//随机选择一个没有冲突的目录名
		string name = RandFileName();
		while (FindName(dir, name) == true) {
			name = RandFileName();
		}
		//更新新目录信息
		fs[i].name = name;
		fs[i].path = fs[dir].path + "/" + name;
		fs[i].fath = dir;
		fs[i].type = 0;
		//更新上级目录信息
		fs[dir].sonfile[fs[dir].sonnum] = i;
		fs[dir].sonnum++;
		if (dir == 0) out << "md" << " " << name << " " << "/" << endl;
		else out << "md" << " " << name << " " << fs[dir].path << endl;
		FileCnt++;
	}

	out.close();
	return;
}

void RandNewfile() {//随机10000个新增文件的命令
	out.open("orders.txt", ios::out | ios::app);
	for (int i = 10001; i < 20001; i++) {
		//随机选择一个可以添加新文件的上级目录
		int dir = rand() % 10001;//目录的范围是[0,10000]
		while (fs[dir].sonnum == 200) {
			dir = rand() % 10001;
		}
		//随机选择一个没有冲突的文件名
		string name = RandFileName();
		while (FindName(dir, name) == true) {
			name = RandFileName();
		}
		//更新新文件信息
		fs[i].name = name;
		fs[i].path = fs[dir].path + "/" + name;
		fs[i].fath = dir;
		fs[i].type = 1;
		//更新上级目录信息
		fs[dir].sonfile[fs[dir].sonnum] = i;
		fs[dir].sonnum++;
		if (dir == 0) out << "newfile" << " " << name << " " << "/" << endl;
		else out << "newfile" << " " << name << " " << fs[dir].path << endl;
		FileCnt++;
	}

	out.close();
	return;
}

void RandCopyhost() {//随机1000个复制主机文件命令
	out.open("orders.txt", ios::out | ios::app);
	int fileindex = 0;
	for (int i = 20001; i < 21001; i++) {
		//随机选择一个可以添加新文件的上级目录
		int dir = rand() % 10001;//目录的范围是[0,10000]
		while (fs[dir].sonnum == 200) {
			dir = rand() % 10001;
		}
		//更新新文件信息
		fs[i].name = to_string(fileindex) + ".txt";
		fs[i].path = fs[dir].path + "/" + fs[i].name;
		fs[i].fath = dir;
		fs[i].type = 1;
		fs[i].fileindex = fileindex;
		//更新上级目录信息
		fs[dir].sonfile[fs[dir].sonnum] = i;
		fs[dir].sonnum++;
		
		string path = "D:\\code\\Linux_ext2\\FileSet\\" + to_string(fileindex) + ".txt";
		if (dir == 0) out << "copy<host>" << " " << path << " " << "/" << " 0" << endl;
		else out << "copy<host>" << " " << path << " " << fs[dir].path << " 0" << endl;
		FileCnt++;
		fileindex = (fileindex + 1) % 1000;
	}

	out.close();
	return;
}

void RandCopylinux() {//随机500个复制主机文件命令
	out.open("orders.txt", ios::out | ios::app);
	for (int i = 21001; i < 21501; i++) {
		//随机选择一个可以添加新文件的上级目录
		int dir = rand() % 10001;//目录的范围是[0,10000]
		while (fs[dir].sonnum == 200) {
			dir = rand() % 10001;
		}
		int file = 10001 + (rand() % 11000);
		while (FindName(dir, fs[file].name) == true) {
			file = 10001 + (rand() % 11000);
		}
		//更新新文件信息
		fs[i].name = fs[file].name;
		fs[i].path = fs[dir].path + "/" + fs[i].name;
		fs[i].fath = dir;
		fs[i].type = 1;
		fs[i].fileindex = fs[file].fileindex;
		//更新上级目录信息
		fs[dir].sonfile[fs[dir].sonnum] = i;
		fs[dir].sonnum++;

		if (dir == 0) out << "copy<lxfs>" << " " << fs[file].path << " " << "/" << endl;
		else out << "copy<lxfs>" << " " << fs[file].path << " " << fs[dir].path << endl;
		FileCnt++;
	}

	out.close();
	return;
}

void Delete() {
	out.open("orders.txt", ios::out | ios::app);
	out << "info" << endl;
	out << "cd /" << endl;
	//out << "ls" << endl;

	for (int i = 0; i < fs[0].sonnum; i++) {
		int file = fs[0].sonfile[i];
		if (fs[file].type == 0) {
			out << "rd /" << fs[file].name << endl;
		}
		else {
			out << "del /" << fs[file].name << endl;
		}
	}
	for (int i = 0; i < 50000; i++) {
		fs[i].clear();
	}
	FileCnt = 0;

	out << "info" << endl;
	out << "cd /" << endl;
	out << "ls" << endl;
	
	out.close();
	return;
}

queue<int> Q;
void Print() {
	out.open("FileTree.txt", ios::out);
	
	Q.push(0);
	while (!Q.empty()) {
		int now = Q.front();
		Q.pop();
		out.setf(ios::left);
		if(now==0) out << setw(15) << fs[now].name << setw(15) << " ";
		else out << setw(15) << fs[now].name << setw(15) << fs[fs[now].fath].name;
		if (fs[now].type == 0) {
			out << setw(10) << "<DIR>";
			if (fs[now].sonnum == 0) {
				out << endl;
				continue;
			}
			for (int i = 0; i < fs[now].sonnum; i++) {
				int sonfile = fs[now].sonfile[i];
				Q.push(sonfile);
				if (i == 0) out << setw(15) << fs[sonfile].name << endl;
				else out << setw(40) << " " << setw(15) << fs[sonfile].name << endl;
			}
		}
		else {
			out << setw(10) << "<FILE>" << setw(15) << fs[now].name << endl;
		}
	}

	out.close();
	return;
}

#endif