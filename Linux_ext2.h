#ifndef Linux_ext2
#define Linux_ext2
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
typedef double db;
const unsigned int INF = 1e9 + 7;
const unsigned int root = 0;
fstream f;
ofstream fout;

const string FileName = "Linux_ext2";                    //文件名
const unsigned int RootDir = 0;                          //根目录的iNode号
const unsigned int BlockSize = 1024;                     //磁盘块大小1024Bytes
const unsigned int iNodeSize = 128;                      //iNode大小128Bytes

const unsigned int TotalBlockNum = 102400;               //100MB空间的总磁盘块数
const unsigned int BootNum = 1;			                 //引导块
const unsigned int SuperNum = 1;			             //超级块
const unsigned int GroupDesNum = 1;		                 //组描述符
const unsigned int BlockBpNum = 16;		                 //块位图
const unsigned int iNodeBpNum = 4;		                 //iNode位图
const unsigned int iNodeTbNum = 4096;	                 //iNode表
const unsigned int DataBlockNum = 98281;       		     //数据块个数

const unsigned int TotaliNodeNum = 32768;                //iNode总数

const unsigned int NameLen = 32;			             //文件名长度上限
const unsigned int FileNum1 = 32768;		             //总文件数量上限
const unsigned int FileNum2 = 256;		                 //每个目录文件下的文件数量上限

unsigned int CurrentPath = RootDir;                      //当前路径，存储iNode编号，初始为根目录

class BootBlock {//1block 1024B 8192b
public:
	unsigned int standby[256];
	
	BootBlock() {
		memset(standby, 0, sizeof(standby));
	}
};

class SuperBlock {//1block 1024B 8192b
public:
	unsigned int tot_block;
	unsigned int tot_datablock, use_datablock;
	unsigned int tot_inode, use_inode;
	unsigned int block_size;
	unsigned int inode_size;
	unsigned int file_num1;
	unsigned int file_num2;
	unsigned int name_len;

	unsigned int standby[246];

	SuperBlock() {
		tot_block = TotalBlockNum;
		tot_datablock = DataBlockNum;
		use_datablock = 1;//根目录占用一个数据块
		tot_inode = TotaliNodeNum;
		use_inode = 1;//根目录占用一个iNode
		block_size = BlockSize;
		inode_size = iNodeSize;
		file_num1 = FileNum1;
		file_num2 = FileNum2;
		name_len = NameLen;

		memset(standby, 0, sizeof(standby));
	}
};

class GroupDescription {//1block 1024B 8192b
public:
	unsigned int group_begin, group_end;
	unsigned int super_begin, super_end;
	unsigned int groupdes_begin, groupdes_end;
	unsigned int blockbm_begin, blockbm_end;
	unsigned int inodebm_begin, inodebm_end;
	unsigned int inodetb_begin, inodetb_end;
	unsigned int data_begin, data_end;

	unsigned int standby[242];

	GroupDescription() {
		group_begin = 1; group_end = 102399;//第0个磁盘块是引导块
		super_begin = 1; super_end = 1;
		groupdes_begin = 2; groupdes_end = 2;
		blockbm_begin = 3; blockbm_end = 18;
		inodebm_begin = 19; inodebm_end = 22;
		inodetb_begin = 23; inodetb_end = 4118;
		data_begin = 4119; data_end = 102399;
	
		memset(standby, 0, sizeof(standby));
	}
};

class BlockBitmap {//16block 16384B 131072b
public:
	unsigned int use[4096];
	BlockBitmap() {
		memset(use, 0, sizeof(use));
		use[0] = 2147483648;//根目录占用第0个bitmap
	}
};

class iNodeBitmap {//4block 4096B 32768b
public:
	unsigned int use[1024];
	iNodeBitmap() {
		memset(use, 0, sizeof(use));
		use[0] = 2147483648;//根目录占用第0个bitmap
	}
};

class iNode {//128B 1024b
public:
	char name[NameLen];           //文件名
	unsigned int last_pos;        //所在目录的iNode号[0,32767]
	unsigned int next_pos;        //下一个iNode号[0,32767]
	unsigned int type;            //type==0目录 type==1二进制文件
	unsigned int files_num;       //如果是目录文件，记录此目录下有多少文件
	unsigned int user_id;         //创建人id
	unsigned int mode;		      //模式 mode==0读写 mode==1只读 
	unsigned int block_num;       //文件占用块数，目录文件最多占一个，普通文件任意
	unsigned int block_pos[17];   //存在数据区的块号 目录文件存此目录下的文件iNode 二进制文件存数据

	iNode() {//构造出的是根目录
		memset(name, '\0', sizeof(name)); name[0]='/';
		last_pos = INF;//INF表示空
		next_pos = INF;
		type = 0;
		files_num = 0;
		user_id = root;
		mode = 0;
		block_num = 1;
		block_pos[0] = 0;//存在第0个数据块中
		for (int i = 1; i < 17; i++) block_pos[i] = INF;
	}
	void clear1() {
		memset(name, '\0', sizeof(name)); name[0] = '/';
		last_pos = INF;
		next_pos = INF;
		type = 0;
		files_num = 0;
		user_id = root;
		mode = 0;
		block_num = 1;
		block_pos[0] = 0;
		for (int i = 1; i < 17; i++) block_pos[i] = INF;
	}
	void clear2() {
		memset(name, '\0', sizeof(name));
		last_pos = INF;
		next_pos = INF;
		type = 0;
		files_num = 0;
		user_id = root;
		mode = 0;
		block_num = 0;
		for (int i = 0; i < 17; i++) block_pos[i] = INF;
	}
};

class iNodeTable {//4096block 4194304B 33554432b
public:
	iNode inode[32768];
};

class Block {//1024B 8192b
public:
	unsigned int data[256];
	Block() {
		memset(data, 0, sizeof(data));
	}
	void clear() {
		memset(data, 0, sizeof(data));
	}
};

class DataBlock {//98281block 100639744B 805117952b
public:
	Block block[98281];
};

class Path {//用于路径分解，方便查找
public:
	unsigned int cnt;
	string  ph[3000];

	Path() {
		cnt = 0;
		for (int i = 0; i < 3000; i++) ph[i] = "";
	}
	void clear() {
		cnt = 0;
		for (int i = 0; i < 3000; i++) ph[i] = "";
	}
}path;

class Order {//用于用户输入的命令分解，方便判断
public:
	unsigned int cnt;
	string od[20];
	unsigned int type;

	Order() {
		cnt = 0;
		for (int i = 0; i < 20; i++) od[i] = "";
		type = INF;
	}
	void clear() {
		cnt = 0;
		for (int i = 0; i < 20; i++) od[i] = "";
		type = INF;
	}
}order;

BootBlock bootblock;
SuperBlock superblock;
GroupDescription groupdes;
BlockBitmap blockmp;
iNodeBitmap inodemp;
iNodeTable inodetb;

queue<Block> buffer;//缓冲区，用于copy<host>命令
queue<unsigned int> qdirinode;//用于ls命令


void Run();//运行程序


void FindOrder(Order& ord);
/*
	Description: 用于获取输入命令
	Input: Order实例化对象ord，存储命令各参数
	Output: 无
	Return: 无
*/
unsigned int FindFirstZero(unsigned int x);
/*
	Description: 找x转化为二进制后的第一个0的位置
	Input: x
	Output: 无
	Return: 从左向右第一个0的位置，[1,32]
*/
unsigned int FindFreeBlock();
/*
	Description: 找到一个空闲的数据块，并在块位图中占用，调用此函数要保证一定有空闲块
	Input: 无
	Output: 无
	Return: 数据块编号，[0,131071]
*/
unsigned int FindFreeINode();
/*
	Description: 找到一个空闲的iNode，并在iNode表中占用
	Input: 无
	Output: 无
	Return: iNode编号，[0,32767]
*/
bool FindDisk();
/*
	Description: 找文件系统的二进制文件
	Input: 无
	Output: 无
	Return: 存在true，不存在false
*/
void FindAbsolutePath(string& relpath);
/*
	Description: 把路径转化为绝对路径
	Input: 输入路径的字符串，绝对路径或是相对路径
	Output: 无
	Return: 无
*/
unsigned int FindFileINode(unsigned int nowdir, string filename);
/*
	Description: 找iNode号为nowdir的目录下，文件名（目录或二进制文件）为filename的iNode号
	Input: nowdir是目录iNode号，filename是要找的文件名
	Output: 无
	Return: 文件名为filename的iNode号，INF表示不存在
*/
unsigned int FindFileINode(string s);
/*
	Description: 根据绝对路径找iNode号
	Input: 绝对路径的字符串
	Output: 无
	Return: 路径文件的iNode号，INF表示路径有误
*/


void ReadFileSystem();
/*
	Description: 读取文件系统的超级块、组描述符、位图和iNode表信息，存入对象
	Input: 无
	Output: 无
	Return: 无
*/
void ReadBlock(unsigned int pos, Block& bk);
/*
	Description: 读取下标为pos的磁盘块信息
	Input: pos是磁盘块下标[0,102399]，bk是要存入的磁盘块对象
	Output: 无
	Return: 无
*/


void WriteFileSystem();
/*
	Description: 将超级块、组描述符、块位图、iNode位图、iNode表写回文件系统
	Input: 无
	Output: 无
	Return: 无
*/
void WriteBlock(unsigned int pos, Block& bk);
/*
	Description: 将磁盘块bk覆盖写到下标为pos的磁盘块中
	Input: pos是磁盘块下标[0,102399]，bk是要写入磁盘的数据
	Output: 无
	Return: 无
*/


void CreateFileSystem();
/*
	Description: 创建文件系统
	Input: 无
	Output: 无
	Return: 无
*/
unsigned int CreateNewFile(unsigned int fathinode, string filename);
/*
	Description: 在iNode为fathinode的目录下创建新文件filename
	Input: fathinode是目录iNode号，filename是子文件（二进制文件）的文件名
	Output: 无
	Return: 新文件的iNode，INF表示新建失败
*/
unsigned int CreateNewDir(unsigned int fathinode, string dirname);
/*
	Description: 在iNode为fathinode的目录下创建新目录dirname
	Input: fathinode是目录iNode号，dirname是目录名
	Output: 无
	Return: 新目录的iNode，INF表示新建失败
*/


void RemoveFileINodeBitmap(unsigned int nowinode);
/*
	Description: 将iNode号为nowinode的文件所占用的iNode位图全部释放（大文件可能释放多个iNode）
	Input: nowinode是文件（二进制文件）的iNode号
	Output: 无
	Return: 无
*/
void RemoveFileBlockBitmap(unsigned int nowinode);
/*
	Description: 将iNode号为nowinode的文件所占用的块位图全部释放
	Input: nowinode是文件（二进制文件）的iNode号
	Output: 无
	Return: 无
*/
void RemoveFileDataBlock(unsigned int nowinode);
/*
	Description: 将iNode号为nowinode的文件所占用的数据块全部释放
	Input: nowinode是文件（二进制文件）的iNode号
	Output: 无
	Return: 无
*/
void RemoveFile(unsigned int nowinode);
/*
	Description: 删除iNode为nowinode的文件
	Input: nowinode是文件（二进制文件）的iNode号
	Output: 无
	Return:  无
*/
void RemoveEmptyDir(unsigned int nowinode);
/*
	Description: 删除iNode为nowinode的目录，此目录保证是空目录
	Input: nowinode是目录的iNode号
	Output: 无
	Return: 无
*/
void RemoveDir(unsigned int nowinode);//删除iNode为nowinode的目录
/*
	Description: 删除iNode为nowinode的目录，此目录下可能有其他文件
	Input: nowinode是目录的iNode号
	Output: 无
	Return: 无
*/

void CatRead(unsigned int nowinode);
/*
	Description: 只读方式打开文件
	Input: nowinode是文件iNode号
	Output: 无
	Return: 无
*/
void CatReadToHost(unsigned int nowinode, string path);
/*
	Description: 只读方式打开文件,并写入主机里
	Input: nowinode是文件iNode号，path是要写入的主机路径
	Output: 无
	Return: 无
*/
void CatWrite(unsigned int nowinode);
/*
	Description: 追加写方式打开文件，若之前的数据块没有用完，追加的内容也不会接着写而是新开一个磁盘块
	Input: nowinode是文件iNode号
	Output: 无
	Return: 无
*/


void CopyHostToBuffer(string hostpath);
/*
	Description: 将要拷贝的主机文件放到缓冲区中
	Input: hostpath是主机文件的路径
	Output: 无
	Return: 无
*/
void CopyBufferToLinux(unsigned int nowinode);
/*
	Description: 将缓冲区的数据写入文件系统,调用此函数要保证iNode和数据块充足
	Input: nowinode是要写入文件的iNode号
	Output: 无
	Return: 无
*/
void CopyLinuxToLinux(unsigned int inode1, unsigned int inode2);
/*
	Description: 文件系统内部的二进制文件拷贝,调用此函数要保证iNode和数据块充足
	Input: inode1是被拷贝的文件iNode，inode2是拷贝的文件iNode
	Output: 无
	Return: 无
*/
void CopyHost(string filename, string hostpath, unsigned int dirinode);
/*
	Description: 把主机路径为hostpath的文件复制到iNode为dirinode的目录中
	Input: filename是主机上的文件名，hostpath是主机文件的路径，是dirnode文件系统中要拷贝进去的目录的iNode号
	Output: 成功或失败的原因
	Return: 无
*/
void CopyLxfs(string filename, unsigned int fileinode, unsigned int dirinode);
/*
	Description: LinuxFileSystem的内部复制
	Input: filename是二进制文件名，fileinode是被拷贝文件的iNode号，dirinode是要拷贝进去的目录的iNode号
	Output: 成功或失败的原因
	Return: 无
*/


void ChangeDir(string newpath);
/*
	Description: 改变当前工作目录
	Input: newpath是要改变成的绝对路径
	Output: 成功或失败
	Return: 无
*/
bool Exist(unsigned int fathinode, string sonname);
/*
	Description: 判断在iNode号为fathinode的目录下有没有名为sonname的文件
	Input: fathinode是目录的iNode号，sonname是文件（目录或二进制文件）的iNode号
	Output: 无
	Return: true存在，false不存在
*/


void ShowHelp();
/*
	Description: 命令提示
	Input: 无
	Output: 无
	Return:无
*/
void ShowInfo();
/*
	Description: 显示整个系统信息
	Input: 无
	Output: 无
	Return: 无
*/
void ShowDir(unsigned int nowdir);
/*
	Description: 显示iNode号为nowdir的目录信息
	Input: nowdir是目录的iNode号
	Output: 目录信息
	Return: 无
*/
void ShowDir(unsigned int nowdir, bool sonfile);
/*
	Description: 显示iNode为nowdir的目录信息，包括子目录名
	Input: nowdir是目录的iNode号，sonfile表示需要显示子目录的信息
	Output: 目录信息
	Return: 无
*/
void ShowList();
/*
	Description: 输出当前文件系统的所有目录与二进制文件信息
	Input: 无
	Output: 目录与文件信息
	Return: 无
*/
void ShowPath();
/*
	Description: 输出当前目录
	Input: 无
	Output: 当前目录
	Return: 无
*/


void Info();
/*
	功能1：info命令，显示整个系统信息
	格式：info
	说明：无
*/
void Cd();
/*
	功能2：cd命令，改变目录
	格式：cd path
	说明：path可以是相对路径或绝对路径
*/
void Dir();
/*
	功能3：dir命令，显示目录
	格式：dir <path> <s>
	说明：path表示指定路径，没有path表示查看当前目录
	      s表示显示所有的子目录，没有s表示不显示
*/
void Md();
/*
	功能4：md命令，创建目录
	格式：md dirname <path>
	说明：dirname是目录名
	      path是指定路径下创建，没有path是在当前路径下创建
*/
void Rd();
/*
	功能5：rd命令，删除目录
	格式：rd path
	说明：path可以是绝对路径或者相对路径
*/
void Newfile();
/*
	功能6：newfile命令，建立文件
	格式：newfile filename <path>
	说明：filename是二进制文件名
	      path是指定路径下创建，没有path是在当前路径下创建
*/
void Cat();
/*
	功能7：cat命令，打开文件
	格式：cat path <r,w>
	说明：path可以是相对路径或绝对路径
	      r表示只读打开，w表示追加写入打开（以回车结束）
*/
void Copy();
/*
	功能8：copy命令，拷贝文件
	格式：copy<host> D:\xxx\yyy\zzz /aaa/bbb <0,1> 或 copy<lxfs> /xxx/yyy/zzz /aaa/bbb
	说明：第一个0是主机文件拷贝到文件系统
		  第一个1是文件系统拷贝到主机文件
	      第二个是文件系统之间拷贝
*/
void Del();
/*
	功能9：del命令，删除文件
	格式：del path
	说明：path可以是绝对路径或者相对路径
*/
void Check();
/*
	功能10：check命令，检测并恢复文件系统
	格式：check
	说明：无
*/
void Ls();
/*
	功能11：ls命令，显示所有目录和二进制文件信息
	格式：ls
	说明：无
*/

#endif