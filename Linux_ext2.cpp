#include"Linux_ext2.h"
#include"TestData.h"
using namespace std;


bool FindDisk() {//找文件系统的二进制文件
	//step1: 打开文件
	f.open(FileName, ios::in | ios::binary);
	//step2: 返回结果 若能打开，需要关闭
	if (!f) return false;
	f.close();
	return true;
}
void FindOrder(Order& ord) {//找输入的命令的含义
	//step1: 清空对象
	ord.clear();
	
	//step2: 按空格分解输入命令字符串
	ReadShareMemory();
	
	ord.cnt = smi.cnt;
	for (int i = 0; i < ord.cnt; i++) {
		ord.od[i] = smi.str[i];
	}

	//step3: 根据第一个字符串确定命令的类型，如果有误，类型为INF
	if (order.od[0] == "exit" || order.od[0] == "EXIT" || order.od[0] == "Exit") order.type = 0;
	else if (order.od[0] == "info")                                              order.type = 1;
	else if (order.od[0] == "cd")                                                order.type = 2;
	else if (order.od[0] == "dir")                                               order.type = 3;
	else if (order.od[0] == "md")                                                order.type = 4;
	else if (order.od[0] == "rd")                                                order.type = 5;
	else if (order.od[0] == "newfile")                                           order.type = 6;
	else if (order.od[0] == "cat")                                               order.type = 7;
	else if (order.od[0] == "copy<host>" || order.od[0] == "copy<lxfs>")         order.type = 8;
	else if (order.od[0] == "del")                                               order.type = 9;
	else if (order.od[0] == "check")                                             order.type = 10;
	else if (order.od[0] == "ls")                                                order.type = 11;
	else                                                                         order.type = INF;

	return;
}
unsigned int FindFirstZero(unsigned int x) {//找x转化为二进制后的第一个0的位置 [1,2,...32]
	//step1: 从低位到高位判断，进行更新
	unsigned int ans = INF;
	for (unsigned int i = 0; i < 32; i++) {
		unsigned int y = (unsigned int)1 << i;
		if ((y & x) == 0) ans = i;
	}

	//step2: 返回结果
	if (ans == INF) return INF;
	return ((unsigned int)32 - ans);
}
unsigned int FindFreeBlock() {//找到一个空闲的数据块，并占用 [保证一定有空闲]
	//step1: 调用FindFirstZero函数找到第一个0的下标并用位运算更新块位图
	unsigned int pos1 = 0, pos2 = 0;
	for (int i = 0; i < 4096; i++) {
		unsigned int pos = FindFirstZero(blockmp.use[i]);
		if (pos != INF) {
			blockmp.use[i] = (blockmp.use[i] | ((unsigned int)1 << ((unsigned int)32 - pos)));
			pos1 = i;
			pos2 = pos;
			break;
		}
	}

	//step2: 更新超级块，返回数据块下标
	superblock.use_datablock++;
	return pos1 * 32 + pos2 - 1;//数据块下标
}
unsigned int FindFreeINode() {//找到一个空闲的iNode，并占用 [保证一定有空闲]
	//step1: 调用FindFirstZero函数找到第一个0的下标并用位运算更新iNode位图
	unsigned int pos1 = 0, pos2 = 0;
	for (int i = 0; i < 1024; i++) {
		unsigned int pos = FindFirstZero(inodemp.use[i]);
		if (pos != INF) {
			inodemp.use[i] = (inodemp.use[i] | ((unsigned int)1 << ((unsigned int)32 - pos)));
			pos1 = i;
			pos2 = pos;
			break;
		}
	}

	//step2: 更新超级块，返回iNode号
	superblock.use_inode++;
	return pos1 * 32 + pos2 - 1;//iNode下标
}
void FindAbsolutePath(string& relpath) {//把路径转化为绝对路径
	/*
	根目录：/
	绝对路径：/xxx/yyy/zzz
	相对路径：./yyy/zzz
	*/
	//step1: 如果是绝对路径，不需操作可以直接返回
	if (relpath[0] == '/') return;

	//step2: 如果是相对路径，需要从下至上一次添加目录名到根目录
	relpath = relpath.substr(1);//把.去除
	unsigned int nowinode = CurrentPath;
	while (1) {
		string str = inodetb.inode[nowinode].name;
		if (str == "/") break;
		else {
			string tmp = inodetb.inode[nowinode].name;
			relpath = "/" + tmp + relpath;
			nowinode = inodetb.inode[nowinode].last_pos;
		}
	}

	return;
}
unsigned int FindFileINode(unsigned int nowdir, string filename) {//根据目录iNode号，找此目录下文件名为filename的iNode号
	//step1: 根据目录的iNode号，确定此目录的信息存在哪个磁盘块中
	unsigned int blockpos = inodetb.inode[nowdir].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;

	//step2: 读取这个磁盘块
	Block db;
	ReadBlock(blockpos, db);

	//step3: 搜索此目录下所有文件的文件名，找不到就返回INF
	for (int i = 0; i < inodetb.inode[nowdir].files_num; i++) {
		unsigned int nowfile = db.data[i];
		if (inodetb.inode[nowfile].name == filename) {
			return nowfile;
		}
	}
	return INF;
}
unsigned int FindFileINode(string s) {//根据绝对路径找iNode号
	//step1: 先将路径分层 /代表根目录 /xxx/yyy/zzz在根目录下找xxx，在xxx目录下找yyy，在yyy目录下找zzz文件
	if (s == "/") return 0; //根目录需要特判
	path.clear();
	string::size_type posl, posr;
	posl = 1;
	posr = s.find("/", posl);
	while (1) {
		if (posr == s.npos) break;
		path.ph[path.cnt++] = s.substr(posl, posr - posl);
		posl = posr + 1;
		posr = s.find("/", posl + 1);
	}
	path.ph[path.cnt++] = s.substr(posl, s.length() - posl);

	//step2: 逐层查找，中间某一个目录或最终文件找不到都代表路径有误
	unsigned nowinode = RootDir;//起点是根目录
	for (int i = 0; i < path.cnt; i++) {
		nowinode = FindFileINode(nowinode, path.ph[i]);
		if (nowinode == INF) return INF;//路径有误
	}

	return nowinode;
}


void ReadFileSystem() {//读取文件系统的超级块、组描述符、位图和iNode表信息
	//step1: 打开文件系统的二进制文件
	f.open(FileName, ios::in | ios::binary);

	//step2: 读取
	f.read((char*)&bootblock, sizeof(BootBlock));       //提取引导块
	f.read((char*)&superblock, sizeof(SuperBlock));     //提取超级块
	f.read((char*)&groupdes, sizeof(GroupDescription)); //提取组描述符
	f.read((char*)&blockmp, sizeof(BlockBitmap));       //提取块位图
	f.read((char*)&inodemp, sizeof(iNodeBitmap));       //提取iNode位图
	f.read((char*)&inodetb, sizeof(iNodeTable));        //提取iNode表

	//step3: 关闭文件
	f.close();
	return;
}
void ReadBlock(unsigned int pos, Block& bk) {//读取下标为pos的磁盘块信息
	//step1: 打开文件系统的二进制文件
	f.open(FileName, ios::in | ios::binary);

	//step2: 计算指针偏移量，并移动读指针
	unsigned int offset = pos * sizeof(Block);
	f.seekg(offset, ios::beg);

	//step3: 读取磁盘块
	f.read((char*)&bk, sizeof(Block));
	
	//step4: 关闭文件
	f.close();
	return;
}
void ReadShareMemory() {//读取一行共享内存中的数据
	//通知shell端希望能输入数据
	pBufIoo->toshell = 1;

	//获取
	while (true) {//等待输入
		if (pBufIoo->tosimdisk == 1) {//shell端通知要输入的数据已全部放入共享内存
			// 打开命名共享内存
			hMapFileIn = OpenFileMapping(
				FILE_MAP_READ,
				FALSE,
				NameIn);
			if (hMapFileIn == NULL) {
				int error = GetLastError();
				_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
				return;
			}

			// 映射对象的一个视图，得到指向共享内存的指针，获取里面的数据
			pBufIn = (ShareMemory*)MapViewOfFile(hMapFileIn,
				FILE_MAP_READ,
				0,
				0,
				BUF_SIZE);
			if (pBufIn == NULL) {
				int error = GetLastError();
				_tprintf(TEXT("Could not map view of file (%d).\n"), error);
				CloseHandle(hMapFileIn);
				return;
			}

			//将数据放入对象
			smi.cnt = pBufIn->cnt;
			for (int i = 0; i < 20; i++) {
				for (int j = 0; j < 300; j++) {
					smi.str[i][j] = pBufIn->str[i][j];
				}
			}

			break;
		}
	}

	//通知shell端数据已读入完成，下面既不要输入也不要输出
	pBufIoo->toshell = 0;
	
	UnmapViewOfFile(pBufIn);
	CloseHandle(hMapFileIn);

	Sleep(10);
	return;
}

void WriteFileSystem() {//将超级块、组描述符、块位图、iNode位图、iNode表写回文件系统
	//step1: 打开文件系统的二进制文件,移动写指针
	f.open(FileName, ios::in | ios::out | ios::binary);
	f.seekp(0, ios::beg);

	//step2: 写入
	f.write((char*)&bootblock, sizeof(BootBlock));       //写入引导块
	f.write((char*)&superblock, sizeof(SuperBlock));     //写入超级块
	f.write((char*)&groupdes, sizeof(GroupDescription)); //写入组描述符
	f.write((char*)&blockmp, sizeof(BlockBitmap));       //写入块位图
	f.write((char*)&inodemp, sizeof(iNodeBitmap));       //写入iNode位图
	f.write((char*)&inodetb, sizeof(iNodeTable));        //写入iNode表

	//step3: 关闭文件
	f.close();
	return;
}
void WriteBlock(unsigned int pos, Block& bk) {//将bk块覆盖写到下标为pos的磁盘块中
	//step1: 打开文件系统的二进制文件
	f.open(FileName, ios::in | ios::out | ios::binary);

	//step2: 计算指针偏移量，并移动写指针
	unsigned int offset = pos * sizeof(Block);
	f.seekp(offset, ios::beg);

	//step3: 写入磁盘块
	f.write((char*)&bk, sizeof(Block));

	//step4: 关闭文件
	f.close();
	return;
}
void WriteShareMemory() {//向输入共享内存存放一行数据
	//创建共享文件
	hMapFileOut = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE,
		NameOut);
	if (hMapFileOut == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
	pBufOut = (ShareMemory*)MapViewOfFile(hMapFileOut,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufOut == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileOut);
		return;
	}

	//清空缓冲区
	pBufOut->cnt = 0;
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 300; j++) {
			pBufOut->str[i][j] = '\0';
		}
	}

	//获取内容，写入共享内存
	pBufOut->cnt = smo.cnt;
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 300; j++) {
			pBufOut->str[i][j] = smo.str[i][j];
		}
	}
	
	//通知shell端希望能把共享内存中的数据输出
	pBufIoo->toshell = 2;
	
	//等shell端全部输出
	while (true) {
		if (pBufIoo->tosimdisk == 2) {//shell端通知已把共享内存中的数据全部输出
			pBufIoo->toshell = 0;//通知shell端现在既不要输入也不要输出
			break;
		}
	}
	
	//清空对象
	smo.clear();

	UnmapViewOfFile(pBufOut);
	CloseHandle(hMapFileOut);

	Sleep(10);
	return;
}

void CreateFileSystem() {//创建文件系统
	//step1: 打开二进制文件
	f.open(FileName, ios::out | ios::binary);

	//step2: 写入各部分数据，其中数据块部分太大，不宜创建对象，采用写入98281个磁盘块的方式
	f.write((char*)&bootblock, sizeof(BootBlock));       //写入引导块
	f.write((char*)&superblock, sizeof(SuperBlock));     //写入超级块
	f.write((char*)&groupdes, sizeof(GroupDescription)); //写入组描述符
	f.write((char*)&blockmp, sizeof(BlockBitmap));       //写入块位图
	f.write((char*)&inodemp, sizeof(iNodeBitmap));       //写入iNode位图
	f.write((char*)&inodetb, sizeof(iNodeTable));        //写入iNode表
	Block block;
	for (int i = 0; i < DataBlockNum; i++) {              //写入数据块
		f.write((char*)&block, sizeof(Block));
	}

	//step3: 关闭文件
	f.close();
	return;
}
unsigned int CreateNewFile(unsigned int fathinode, string filename) {//在iNode为fathinode的目录下创建新文件filename
	//step1: 查看iNode剩余数量、数据块剩余数量是否可以新建文件
	if (superblock.use_inode == superblock.tot_inode) return INF;
	if (superblock.use_datablock == superblock.tot_datablock) return INF;
	if (inodetb.inode[fathinode].files_num == superblock.file_num2) return INF;

	//step2: 在iNode位图中找到第一个0，确定要分配的iNode号,并分配
	unsigned int soninode = FindFreeINode();

	//step3: 更新父目录iNode表及数据块中的数据
	inodetb.inode[fathinode].files_num++;
	unsigned int blockpos = inodetb.inode[fathinode].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db;
	ReadBlock(blockpos, db);
	db.data[inodetb.inode[fathinode].files_num - 1] = soninode;
	WriteBlock(blockpos, db);

	//step4: 更新子文件iNode中的数据
	char tmpname[NameLen]; memset(tmpname, '\0', sizeof(tmpname));
	strcpy_s(tmpname, filename.c_str());
	strcpy_s(inodetb.inode[soninode].name, tmpname);   //文件名，需要把string转化为字符数组
	inodetb.inode[soninode].last_pos = fathinode;      //父目录的iNode号
	inodetb.inode[soninode].next_pos = INF;		       //下一个iNode号，用于超过17KB的文件，新建的文件为空所以用不上
	inodetb.inode[soninode].type = 1;				   //指明是普通文件
	inodetb.inode[soninode].files_num = 0;			   //普通文件不会再有子文件，所以为0
	inodetb.inode[soninode].user_id = root;		       //先假设是root创建的
	inodetb.inode[soninode].mode = 0;				   //先假设是可读可写的目录
	inodetb.inode[soninode].block_num = 0;			   //空的普通文件不占用数据块

	//step5: 更新DataBlock中的数据，由于是空文件，所以不用任何操作

	//step6: 将超级块等信息回文件系统，保证数据一致性
	WriteFileSystem();

	//step7:更新当前路径
	CurrentPath = fathinode;

	//step8: 返回子文件的iNode
	return soninode;
}
unsigned int CreateNewDir(unsigned int fathinode, string dirname) {//创建新目录，fathinode保证存在有效
	//step1: 查看iNode剩余数量、数据块剩余数量是否可以新建文件 看父目录的子目录数量是否达到上限
	if (superblock.use_inode  == superblock.tot_inode) return INF;
	if (superblock.use_datablock  == superblock.tot_datablock) return INF;
	if (inodetb.inode[fathinode].files_num == superblock.file_num2) return INF;

	//step2: 在iNode位图中找到第一个0，确定要分配的iNode号,并分配
	unsigned int soninode = FindFreeINode();

	//step3: 在块位图中找到第一个0，确定要分配的数据块，并分配
	unsigned int datablock = FindFreeBlock();
	

	//step4: 更新父目录iNode表及数据块中的数据
	inodetb.inode[fathinode].files_num++;
	unsigned int blockpos = inodetb.inode[fathinode].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db;
	ReadBlock(blockpos, db);
	db.data[inodetb.inode[fathinode].files_num - 1] = soninode;
	WriteBlock(blockpos, db);

	//step5: 更新子目录iNode中的数据
	char tmpname[NameLen]; memset(tmpname, '\0', sizeof(tmpname));
	strcpy_s(tmpname, dirname.c_str());
	strcpy_s(inodetb.inode[soninode].name, tmpname);   //文件名，需要把string转化为字符数组
	inodetb.inode[soninode].last_pos = fathinode;      //父目录的iNode号
	inodetb.inode[soninode].next_pos = INF;		       //下一个iNode号，用于超过17KB的文件，目录文件是用不上的
	inodetb.inode[soninode].type = 0;				   //指明是目录文件
	inodetb.inode[soninode].files_num = 0;			   //此目录下还没有子目录或文件
	inodetb.inode[soninode].user_id = root;		       //先假设是root创建的
	inodetb.inode[soninode].mode = 0;				   //先假设是可读可写的目录
	inodetb.inode[soninode].block_num = 1;			   //目录文件只占一个数据块用于存储子目录和文件的iNode
	inodetb.inode[soninode].block_pos[0] = datablock;  //记录此目录的数据块iNode号

	//step6: 更新DataBlock中的数据，由于是空目录，所以不用任何操作

	//step7:将超级块等信息回文件系统，保证数据一致性
	WriteFileSystem();

	//step8: 更新当前路径
	CurrentPath = soninode;

	//step9: 返回子目录的iNode
	return soninode;
}


void RemoveFileINodeBitmap(unsigned int nowinode) {//将iNode号为nowinode的文件所占用的iNode位图全部释放
	//step1: 把iNode位图释放
	unsigned int pos1 = 0, pos2 = 0;
	pos1 = nowinode / 32;
	pos2 = nowinode % 32;
	unsigned int h = (1 << ((unsigned int)31 - pos2));
	h = (~h);
	inodemp.use[pos1] = (inodemp.use[pos1] & h);
	superblock.use_inode--;
	
	//step2: 如果此文件占用多个iNode，也要清空
	if (inodetb.inode[nowinode].next_pos != INF) {
		RemoveFileINodeBitmap(inodetb.inode[nowinode].next_pos);
	}

	return;
}
void RemoveFileBlockBitmap(unsigned int nowinode) {//将iNode号为nowinode的文件所占用的块位图全部释放
	//step1: 把块位图释放
	for (int i = 0; i < inodetb.inode[nowinode].block_num; i++) {
		unsigned int pos1 = 0, pos2 = 0;
		unsigned int datablockpos = inodetb.inode[nowinode].block_pos[i];
		pos1 = datablockpos / 32;
		pos2 = datablockpos % 32;
		unsigned int h = (1 << ((unsigned int)31 - pos2));
		h = (~h);
		blockmp.use[pos1] = (blockmp.use[pos1] & h);
		superblock.use_datablock--;
	}
	//step2: 如果此文件占用多个iNode，对应的数据块也要清空
	if (inodetb.inode[nowinode].next_pos != INF) {
		RemoveFileBlockBitmap(inodetb.inode[nowinode].next_pos);
	}

	return;
}
void RemoveFileDataBlock(unsigned int nowinode) {//将iNode号为nowinode的文件所占用的数据块全部释放
	//step1: 把每一个数据块都清空
	Block db;
	for (int i = 0; i < inodetb.inode[nowinode].block_num; i++) {
		unsigned int blockpos = inodetb.inode[nowinode].block_pos[i];
		blockpos = groupdes.data_begin + blockpos;
		WriteBlock(blockpos, db);
	}

	//step2: 如果此文件占用多个iNode，也要清空
	if (inodetb.inode[nowinode].next_pos != INF) {
		RemoveFileDataBlock(inodetb.inode[nowinode].next_pos);
	}

	return;
}
void RemoveFile(unsigned int nowinode) {//删除iNode为nowinode的普通文件
	//step1: 将数据块中的数据释放,也就是存此目录信息的数据块数据全部置0
	RemoveFileDataBlock(nowinode);

	//step2: 更新父目录iNode表及数据块中的数据
	unsigned int fathinode = inodetb.inode[nowinode].last_pos;
	unsigned int blockpos = inodetb.inode[fathinode].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db;
	ReadBlock(blockpos, db);
	for (int i = 0, jud = 0; i < inodetb.inode[fathinode].files_num; i++) {
		if (jud == 1) {
			db.data[i] = db.data[i + 1];
		}
		else if (db.data[i] == nowinode) {
			jud = 1;
			db.data[i] = db.data[i + 1];
		}
	}
	WriteBlock(blockpos, db);
	inodetb.inode[fathinode].files_num--;

	//step3: 更新块位图
	RemoveFileBlockBitmap(nowinode);

	//step4: 更新iNode位图
	RemoveFileINodeBitmap(nowinode);

	//step5: 更新自己的iNodeTable，清空
	inodetb.inode[nowinode].clear1();

	//step6: 将超级块等信息回文件系统，保证数据一致性
	WriteFileSystem();

	return;
}
void RemoveEmptyDir(unsigned int nowinode) {//删除iNode为nowinode的目录，此目录保证是空目录
	//step1: 将数据块中的数据释放,也就是存此目录信息的数据块数据全部置0
	unsigned int blockpos = inodetb.inode[nowinode].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db1;
	WriteBlock(blockpos, db1);

	//step2: 更新父目录iNode表及数据块中的数据
	unsigned int fathinode = inodetb.inode[nowinode].last_pos;
	blockpos= inodetb.inode[fathinode].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db2;
	ReadBlock(blockpos, db2);
	for (int i = 0, jud = 0; i < inodetb.inode[fathinode].files_num; i++) {
		if (jud == 1) {
			db2.data[i] = db2.data[i + 1];
		}
		else if (db2.data[i] == nowinode) {
			jud = 1;
			db2.data[i] = db2.data[i + 1];
		}
	}
	WriteBlock(blockpos, db2);
	inodetb.inode[fathinode].files_num--;

	//step3: 更新块位图
	unsigned int pos1 = 0, pos2 = 0;
	unsigned int datablockpos = inodetb.inode[nowinode].block_pos[0];
	pos1 = datablockpos / 32;
	pos2 = datablockpos % 32;
	unsigned int h = (1 << ((unsigned int)31 - pos2));
	h = (~h);
	blockmp.use[pos1] = (blockmp.use[pos1] & h);
	superblock.use_datablock--;

	//step4: 更新iNode位图
	unsigned int pos3 = 0, pos4 = 0;
	pos3 = nowinode / 32;
	pos4 = nowinode % 32;
	h = (1 << ((unsigned int)31 - pos4));
	h = (~h);
	inodemp.use[pos3] = (inodemp.use[pos3] & h);
	superblock.use_inode--;

	//step5: 更新自己的iNodeTable，清空
	inodetb.inode[nowinode].clear1();

	//step6: 将超级块等信息回文件系统，保证数据一致性
	WriteFileSystem();

	return;
}
void RemoveDir(unsigned int nowinode) {//删除iNode为nowinode的目录
	//step1: 如果是普通文件，直接调用函数删除；如果是目录文件，空则直接删除，非空递归删除
	if (inodetb.inode[nowinode].type == 1) {//普通文件
		RemoveFile(nowinode);
		return;
	}
	else {//目录文件
		if (inodetb.inode[nowinode].files_num == 0) {
			RemoveEmptyDir(nowinode);
			return;
		}
		else {
			unsigned int blockpos = inodetb.inode[nowinode].block_pos[0];
			blockpos = groupdes.data_begin + blockpos;
			Block db;
			ReadBlock(blockpos, db);

			while (inodetb.inode[nowinode].files_num != 0) {
				ReadBlock(blockpos, db);
				unsigned int nowfile = db.data[0];
				RemoveDir(nowfile);
			}
			RemoveEmptyDir(nowinode);
		}
	}
	return;
}


void CatRead(unsigned int nowinode) {
	Block db;
	for (;;) {
		//step1: 枚举当前iNode下所有块的数据
		for (int i = 0; i < inodetb.inode[nowinode].block_num; i++) {
			unsigned int blockpos = inodetb.inode[nowinode].block_pos[i];
			blockpos = groupdes.data_begin + blockpos;
			ReadBlock(blockpos, db);
			for (int j = 0; j < 256; j++) {
				char c1, c2, c3, c4;
				c1 = char((db.data[j] & (unsigned int)4278190080) >> 24);
				c2 = char((db.data[j] & (unsigned int)16711680) >> 16);
				c3 = char((db.data[j] & (unsigned int)65280) >> 8);
				c4 = char(db.data[j] & (unsigned int)255);
				if (c1 == '\0' && c2 == '\0' && c3 == '\0' && c4 == '\0') {//连续4个空格表示内容结束
					break;
				}
				
				smo.cnt = 1;
				smo.str[0][0] = c1; smo.str[0][1] = c2; smo.str[0][2] = c3; smo.str[0][3] = c4;
				WriteShareMemory();
			}
		}

		//step2: 考虑大文件可能占有多个iNode
		if (inodetb.inode[nowinode].next_pos == INF) break;
		nowinode = inodetb.inode[nowinode].next_pos;
	}
	smo.cnt = 1;
	smo.str[0][0] = '\n';
	WriteShareMemory();
	
	return;
}
void CatReadToHost(unsigned int nowinode, string path) {
	Block db;
	path = path + "/" + inodetb.inode[nowinode].name;
	fout.open(path, ios::out);
	for (;;) {
		//step1: 枚举当前iNode下所有块的数据
		for (int i = 0; i < inodetb.inode[nowinode].block_num; i++) {
			unsigned int blockpos = inodetb.inode[nowinode].block_pos[i];
			blockpos = groupdes.data_begin + blockpos;
			ReadBlock(blockpos, db);
			for (int j = 0; j < 256; j++) {
				char c1, c2, c3, c4;
				c1 = char((db.data[j] & (unsigned int)4278190080) >> 24);
				c2 = char((db.data[j] & (unsigned int)16711680) >> 16);
				c3 = char((db.data[j] & (unsigned int)65280) >> 8);
				c4 = char(db.data[j] & (unsigned int)255);
				if (c1 == '\0' && c2 == '\0' && c3 == '\0' && c4 == '\0') {//连续4个空格表示内容结束
					break;
				}
				fout << c1 << c2 << c3 << c4;
			}
		}

		//step2: 考虑大文件可能占有多个iNode
		if (inodetb.inode[nowinode].next_pos == INF) break;
		nowinode = inodetb.inode[nowinode].next_pos;
	}
	
	f.close();
	return;
}
void CatWrite(unsigned int nowinode) {
	//step1: 读入一行字符串
	smo.cnt = 1;
	smo.str[0][0] = '\0'; smo.str[0][1] = '\0';
	WriteShareMemory();

	string str;
	ReadShareMemory();
	for (int i = 0; i < smi.cnt - 1; i++) {
		str = str + smi.str[i] + " ";
	}
	str = str + smi.str[smi.cnt - 1];

	//step2: 判断iNode或者数据块够不够
	unsigned int Size = sizeof(str);
	unsigned int needblock = 0, needinode = 0;
	needblock = Size / sizeof(Block);
	if (Size % sizeof(Block) != 0) needblock++;
	needinode = needblock / 17;
	if (needblock % 17 != 0) needinode++;
	
	if (superblock.use_datablock + needblock > superblock.tot_datablock) {//数据块数量不足，拷贝失败
		string tmps = "  Datablock has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {//iNode数量不足，拷贝失败
		string tmps = "  iNode has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}

	//step3: 将字符串补成4的整数倍，方便写入缓冲区
	unsigned int len = str.length();
	if (len % 4 == 1) str = str + "   ";
	else if (len % 4 == 2) str = str + "  ";
	else if (len % 4 == 3) str = str + " ";
	len = str.length();

	//step3: 写入缓冲区
	Block db;
	int cnt = 0;
	for (int i = 0; i < needblock; i++) {
		db.clear();
		for (int j = 0; j < 256; j++) {
			db.data[j] = (unsigned int)(str[cnt++] << 24);
			db.data[j] = (db.data[j] | ((unsigned int)(str[cnt++] << 16)));
			db.data[j] = (db.data[j] | ((unsigned int)(str[cnt++] << 8)));
			db.data[j] = (db.data[j] | ((unsigned int)(str[cnt++])));
			if (cnt == len) break;
		}
		buffer.push(db);
	}

	//step4: 将缓冲区追加写入文件系统
	CopyBufferToLinux(nowinode);
	string tmps = "  CatWrite order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();
	
	//step5: 更新文件系统数据
	WriteFileSystem();

	return;
}


void CopyHostToBuffer(string hostpath) {//将要拷贝的文件放到缓冲区中
	//step1: 清空缓冲区
	while (!buffer.empty()) buffer.pop();

	//step2: 获得文件总大小、需要占用的完整磁盘块数、需要占用的总块数
	unsigned int filesize = 0, blockneed1 = 0, blockneed2 = 0;
	f.open(hostpath, ios::in);
	f.seekg(0, ios::end);
	filesize = f.tellg();
	blockneed1 = filesize / sizeof(Block);
	if (filesize % sizeof(Block) == 0) blockneed2 = blockneed1;
	else blockneed2 = blockneed1 + 1;
	f.seekg(0, ios::beg);

	//step3: 读完整的磁盘块
	Block db1;
	for (int i = 0; i < blockneed1; i++) {
		for (int j = 0; j < 256; j++) {
			char c1, c2, c3, c4;
			f.read((char*)&c1, sizeof(char));
			f.read((char*)&c2, sizeof(char));
			f.read((char*)&c3, sizeof(char));
			f.read((char*)&c4, sizeof(char));
			db1.data[j] = (((unsigned int)c1 << 24) | ((unsigned int)c2 << 16) | ((unsigned int)c3 << 8) | (unsigned int)c4);
		}
		buffer.push(db1);
	}

	//step4: 剩余的数据按unsigned int类型读入
	Block db2;
	unsigned int remain = filesize % sizeof(Block);
	unsigned int intneed = remain / sizeof(unsigned int), charneed = remain % sizeof(unsigned int);
	for (int i = 0; i < intneed; i++) {
		char c1, c2, c3, c4;
		f.read((char*)&c1, sizeof(char));
		f.read((char*)&c2, sizeof(char));
		f.read((char*)&c3, sizeof(char));
		f.read((char*)&c4, sizeof(char));
		db2.data[i] = (((unsigned int)c1 << 24) | ((unsigned int)c2 << 16) | ((unsigned int)c3 << 8) | (unsigned int)c4);
	}

	if (charneed != 0) {
		char c1, c2, c3;
		if (charneed >= 1) f.read((char*)&c1, sizeof(char));
		if (charneed >= 2) f.read((char*)&c2, sizeof(char));
		if (charneed >= 3) f.read((char*)&c3, sizeof(char));

		if (charneed == 1) db2.data[intneed] = ((unsigned int)c1 << 24);
		else if (charneed == 2) db2.data[intneed] = (((unsigned int)c1 << 24) | ((unsigned int)c2 << 16));
		else if (charneed == 3) db2.data[intneed] = (((unsigned int)c1 << 24) | ((unsigned int)c2 << 16) | ((unsigned int)c3 << 8));
	}
	buffer.push(db2);

	//step5: 关闭文件
	f.close();
}
void CopyBufferToLinux(unsigned int nowinode) {//将缓冲区追加写入文件系统
	//step1: 原文件可能占有多个iNode，需要找到最后一个
	while (inodetb.inode[nowinode].next_pos != INF)
		nowinode = inodetb.inode[nowinode].next_pos;

	//step2: 写入文件系统
	Block db;
	while (!buffer.empty()) { //缓冲区不为空
		if (inodetb.inode[nowinode].block_num == 17) {//此iNode已满
			unsigned int nextinode = FindFreeINode();//下一个文件的iNode下标
			inodetb.inode[nextinode].clear2();
			inodetb.inode[nowinode].next_pos = nextinode;//形成链表结构
			
			return CopyBufferToLinux(nextinode);
		}

		//取数据
		db = buffer.front();  //取队首
		buffer.pop();         //弹出队首
		
		//分配数据块
		unsigned int datablock = FindFreeBlock();        //分配的数据块下标
		WriteBlock(groupdes.data_begin + datablock, db); //写入数据块
		
		//更新iNode表
		unsigned int pos = inodetb.inode[nowinode].block_num++;
		inodetb.inode[nowinode].block_pos[pos] = datablock;
	}

	return;
}
void CopyHost(string filename, string hostpath, unsigned int dirinode) {//把主机路径为hostpath,文件名为filename的文件拷贝到iNode为dirinode的目录中
	//step1: 将所有文件数据放入缓冲区中
	CopyHostToBuffer(hostpath);

	//step2: 判断iNode或者数据块够不够
	unsigned int needblock = buffer.size();
	unsigned int needinode = needblock / 17;
	if (needblock % 17 != 0) needinode++;

	if (superblock.use_datablock + needblock > superblock.tot_datablock) {//数据块数量不足，拷贝失败
		string tmps = "  Datablock has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {//iNode数量不足，拷贝失败
		string tmps = "  iNode has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	
	//step3: 在dirinode下新建一个文件
	unsigned int nowinode = CreateNewFile(dirinode, filename);

	//step4: 将缓冲区的块放入文件系统
	CopyBufferToLinux(nowinode);
	string tmps = "  CopyHost order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();

	//step5: 更新文件系统数据
	WriteFileSystem();

	return;
}
void CopyLinuxToLinux(unsigned int inode1, unsigned int inode2) {//文件系统内部的二进制文件拷贝
	//step1: 占用数据块个数一样
	inodetb.inode[inode2].block_num = inodetb.inode[inode1].block_num;
	
	//step2: 复制具体数据
	Block db;
	for (int i = 0; i < inodetb.inode[inode1].block_num; i++) {
		unsigned int blockpos = inodetb.inode[inode1].block_pos[i];// 读取数据
		blockpos = groupdes.data_begin + blockpos;
		ReadBlock(blockpos, db);

		unsigned int datablock = FindFreeBlock();  //分配的数据块下标
		inodetb.inode[inode2].block_pos[i] = datablock; //更新iNode表
		WriteBlock(groupdes.data_begin + datablock, db);//写入数据块
	}

	if (inodetb.inode[inode1].next_pos != INF) {
		unsigned int nextinode = FindFreeINode();  //下一个文件的iNode下标
		inodetb.inode[nextinode].clear2();
		inodetb.inode[inode2].next_pos = nextinode;

		return CopyLinuxToLinux(inodetb.inode[inode1].next_pos, inodetb.inode[inode2].next_pos);
	}

	return;
}
void CopyLxfs(string filename, unsigned int fileinode, unsigned int dirinode) {//LinuxFileSystem的内部拷贝
	//step1: 判断iNode或者数据块够不够
	unsigned int needblock = 0, needinode = 0;
	unsigned int nowinode = fileinode;
	for (;;) {
		needblock += inodetb.inode[nowinode].block_num;
		needinode += 1;
		if (inodetb.inode[nowinode].next_pos == INF) break;
		nowinode = inodetb.inode[nowinode].next_pos;
	}

	if (superblock.use_datablock + needblock > superblock.tot_datablock) {//数据块数量不足，拷贝失败
		string tmps = "  Datablock has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {//iNode数量不足，拷贝失败
		string tmps = "  iNode has been used up, copy failed!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}

	//step2: 在dirinode下新建一个文件
	nowinode = CreateNewFile(dirinode, filename);

	//step3: 复制
	CopyLinuxToLinux(fileinode, nowinode);
	string tmps = "  CopyLxfs order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();

	//step4: 更新文件系统数据
	WriteFileSystem();
	
	return;
}


void ChangeDir(string newpath) {//改变当前工作目录，newpath是绝对路径
	unsigned int x = FindFileINode(newpath);
	if (x != INF) {
		CurrentPath = x;
		string tmps = "  ChangeDir order executed successfully!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	else {
		string tmps = "  ChangeDir order executed unsuccessfully!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
}
bool Exist(unsigned int fathinode, string sonname) {//判断在iNode号为fathinode的目录下有没有名为sonname的文件
	//step1: 直接调用函数找iNode，找不到就是不存在
	unsigned int x = FindFileINode(fathinode, sonname);
	if (x != INF) return true;
	return false;
}


void ShowHelp() {//命令提示
	string tmps[20]; 
	tmps[0] =  "***************************Linux ext2 FileSystem***************************\n\n";
	tmps[1] =  "    info                                          Show FileSystem Information\n";
	tmps[2] =  "    cd path                                       Change Directory\n";
	tmps[3] =  "    dir <path> <s>                                Show Directory\n";
	tmps[4] =  "    md dirname <path>                             Create Directory\n";
	tmps[5] =  "    rd path                                       Remove Directory\n";
	tmps[6] =  "    newfile filename <path>                       Create File\n";
	tmps[7] =  "    cat path <r,w>                                Open File\n";
	tmps[8] =  "    copy<host> HostPath LinuxPath <0,1>           Copy Host File\n";
	tmps[9] =  "    copy<lxfs> LinuxPath1 LinuxPath2              Copy Linux File\n";
	tmps[10] = "    del path                                      Delete File\n";
	tmps[11] = "    check                                         Check&Recovery System\n";
	tmps[12] = "    ls                                            Show File List\n";
	tmps[13] = "\n";
	
	smo.cnt = 14;
	for (int i = 0; i < 14; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();

	return;
}
void ShowInfo() {//显示整个系统信息
	string tmps[30];
	tmps[0] =  "************************Linux ext2 FileSystem************************\n\n";
	tmps[1] =  "Information:\n";
	tmps[2] =  "    Total Disk Size:                          100MB\n";
	tmps[3] =  "    Block Size:                               " + to_string(superblock.block_size) + "B\n";
	tmps[4] =  "    iNode Size:                               " + to_string(superblock.inode_size) + "B\n";
	tmps[5] =  "    Group Number:                             1\n";
	tmps[6] =  "    Total Blocks Number:                      " + to_string(superblock.tot_block) + "\n";
	tmps[7] =  "    BootBlock Blocks Number:                  " + to_string(groupdes.super_begin) + "\n";
	tmps[8] =  "    SuperBlock Blocks Number:                 " + to_string(groupdes.super_end - groupdes.super_begin + 1) + "\n";
	tmps[9] =  "    GroupDescriptionBlock Block Number:       " + to_string(groupdes.groupdes_end - groupdes.groupdes_begin + 1) + "\n";
	tmps[10] = "    DataBlockBitMap Blocks Number:            " + to_string(groupdes.blockbm_end - groupdes.blockbm_begin + 1) + "\n";
	tmps[11] = "    iNodeBitMap Blocks Number:                " + to_string(groupdes.inodebm_end - groupdes.inodebm_begin + 1) + "\n";
	tmps[12] = "    iNodeTable Blocks Number:                 " + to_string(groupdes.inodetb_end - groupdes.inodetb_begin + 1) + "\n";
	tmps[13] = "    DataBlock Blocks Number:                  " + to_string(groupdes.data_end - groupdes.data_begin + 1) + "\n";
	tmps[14] = "\n";
	tmps[15] = "    Used iNode Number:                        " + to_string(superblock.use_inode) + "\n";
	tmps[16] = "    Used DataBlock Number:                    " + to_string(superblock.use_datablock) +  "\n";
	tmps[17] = "\n";

	smo.cnt = 18;
	for (int i = 0; i < 18; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();
	

	return;
}
void ShowDir(unsigned int nowdir) {//显示iNode号为nowdir的目录信息
	string tmps[10];
	tmps[0] = "   Directory Name:    " + string(inodetb.inode[nowdir].name) + "\n";
	tmps[1] = "   Block Position:    " + to_string(groupdes.data_begin + inodetb.inode[nowdir].block_pos[0]) + "\n";
	tmps[2] = "   File Length:       128B(iNode) + 1024B(DataBlock) = 1152B\n\n";
	
	smo.cnt = 3;
	for (int i = 0; i < 3; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();
	
	CurrentPath = nowdir;

	return;
}
void ShowDir(unsigned int nowdir, bool sonfile) {//显示iNode为nowdir的目录信息，包括子目录名
	string tmps[10];
	tmps[0] = "    Directory Name:          " + string(inodetb.inode[nowdir].name) + "\n";
	tmps[1] = "    Block Position:          " + to_string(groupdes.data_begin + inodetb.inode[nowdir].block_pos[0]) + "\n";
	tmps[2] = "    File Length:             128B + 1024B = 1152B\n";
	
	smo.cnt = 3;
	for (int i = 0; i < 3; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();
	

	unsigned int blockpos = inodetb.inode[nowdir].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db;
	ReadBlock(blockpos, db);

	tmps[0] = "    SubDirectory/SubFile Number:    " + to_string(inodetb.inode[nowdir].files_num) + "\n";
	tmps[1] = "    SubDirectory Name:  ";
	smo.cnt = 2;
	for (int i = 0; i < 2; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();
	

	for (int i = 0; i < inodetb.inode[nowdir].files_num; i++) {
		unsigned int nowfile = db.data[i];
		if (inodetb.inode[nowfile].type == 0) {
			tmps[0] = string(inodetb.inode[nowfile].name) + " <DIR>   ";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
		}
		else {
			tmps[0] = string(inodetb.inode[nowfile].name) + " <FILE>   ";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
		}
	}
	tmps[0] = "\n\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps[0].c_str());
	WriteShareMemory();

	CurrentPath = nowdir;

	return;
}
void ShowList() {
	Block db;
	while (!qdirinode.empty()) qdirinode.pop();
	qdirinode.push(RootDir);

	char tmps[10][300]; 
	memset(tmps, '\0', sizeof(tmps));
	sprintf_s(tmps[0], "  %-25s%-22s%-10s%-12s%-25s\n", "Directory/File Name", "Parent Directory", "Type", "Blocks", "SubDirectory/SubFile");
	smo.cnt = 1;
	for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
	WriteShareMemory();

	while (!qdirinode.empty()) {
		unsigned int nowinode = qdirinode.front();
		unsigned int fathinode = inodetb.inode[nowinode].last_pos;
		qdirinode.pop();

		if (inodetb.inode[nowinode].type == 0) {//目录文件
			
			if (nowinode == RootDir) {//根目录
				memset(tmps[0], '\0', sizeof(tmps[0]));
				sprintf_s(tmps[0], "  %-25s%-22s%-10s%-12s", inodetb.inode[nowinode].name, " ", "<DIR>", "1");
				smo.cnt = 1;
				for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
				WriteShareMemory();
			}
			else {
				memset(tmps[0], '\0', sizeof(tmps[0]));
				sprintf_s(tmps[0], "  %-25s%-22s%-10s%-12s", inodetb.inode[nowinode].name, inodetb.inode[fathinode].name, "<DIR>", "1");
				smo.cnt = 1;
				for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
				WriteShareMemory();
			}
			unsigned int blockpos = inodetb.inode[nowinode].block_pos[0];
			blockpos = groupdes.data_begin + blockpos;
			ReadBlock(blockpos, db);
			for (int i = 0, cnt = 0; i < inodetb.inode[nowinode].files_num; i++, cnt++) {
				unsigned int soninode = db.data[i];
				qdirinode.push(soninode);
				if (cnt == 0) {
					memset(tmps[0], '\0', sizeof(tmps[0]));
					sprintf_s(tmps[0], "%-12s\n", inodetb.inode[soninode].name);
					smo.cnt = 1;
					for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
					WriteShareMemory();
				}
				else {
					memset(tmps[0], '\0', sizeof(tmps[0]));
					sprintf_s(tmps[0], "%-71s%s\n", " ", inodetb.inode[soninode].name);
					smo.cnt = 1;
					for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
					WriteShareMemory();
				}
			}
			if (inodetb.inode[nowinode].files_num == 0) {
				memset(tmps[0], '\0', sizeof(tmps[0]));
				tmps[0][0] = '\n';
				smo.cnt = 1;
				for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
				WriteShareMemory();
			}
		}
		else {//普通二进制文件
			memset(tmps[0], '\0', sizeof(tmps[0]));
			sprintf_s(tmps[0], "  %-25s%-22s%-10s", inodetb.inode[nowinode].name, inodetb.inode[fathinode].name, "<FILE>");
			smo.cnt = 1;
			for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
			WriteShareMemory();

			unsigned int blocknum = 0;
			unsigned int tmpinode = nowinode;
			for (;;) {
				blocknum += inodetb.inode[tmpinode].block_num;
				if (inodetb.inode[tmpinode].next_pos == INF) break;
				tmpinode = inodetb.inode[tmpinode].next_pos;
			}
			memset(tmps[0], '\0', sizeof(tmps[0]));
			_itoa_s(blocknum, tmps[0], 10);
			sprintf_s(tmps[0], "%-12s\n", tmps[0]);
			smo.cnt = 1;
			for (int i = 0; i < 300; i++) smo.str[0][i] = tmps[0][i];
			WriteShareMemory();
		}
	}

	return;
}
void ShowPath() {
	string str = inodetb.inode[CurrentPath].name;
	str = "  " + str + ">$ ";
	smo.cnt = 1;
	strcpy_s(smo.str[0], str.c_str());
	WriteShareMemory();

	return;
}

void Info() {//info
	Reader1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	ShowInfo();

	Reader2();
	return;
}
void Cd() {//cd path
	Reader1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	FindAbsolutePath(order.od[1]);
	ChangeDir(order.od[1]);
	
	Reader2();
	return;
}
void Dir() {//dir 或 dir s 或 dir path 或 dir path s
	Reader1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	if (order.cnt == 1) {//当前目录，不显示子目录
		ShowDir(CurrentPath);
	}
	else if (order.cnt == 2) {
		if (order.od[1] == "s")//当前目录，显示子目录
			ShowDir(CurrentPath, true);
		else {//指定目录，不显示子目录
			FindAbsolutePath(order.od[1]);
			unsigned int inode = FindFileINode(order.od[1]);
			if (inode == INF) {
				string tmps;
				tmps = "  Failed: The path is not exist!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
			}
			else ShowDir(inode);
		}
	}
	else if (order.cnt == 3) {//指定目录，显示子目录
		FindAbsolutePath(order.od[1]);
		unsigned int inode = FindFileINode(order.od[1]);
		if (inode == INF) {
			string tmps;
			tmps = "  Failed: The path is not exist!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
		}
		else ShowDir(inode, true);
	}

	Reader2();
	return;
}
void Md() {//md dirname 或 md dirname path
	Writer1();
	
	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	if (order.cnt == 2) {//md dirname 在当前目录下新建目录
		if (Exist(CurrentPath, order.od[1]) == true) {
			string tmps;
			tmps = "  Failed: The Directory already existed!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
			return;
		}
		CreateNewDir(CurrentPath, order.od[1]);
	}
	else {//md dirname path 在path所在的目录下新建目录
		FindAbsolutePath(order.od[2]);
		unsigned int inode = FindFileINode(order.od[2]);
		if (inode == INF) {
			string tmps;
			tmps = "  Failed: The path is not exist!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
		}
		else {
			if (Exist(inode, order.od[1]) == true) {
				string tmps;
				tmps = "  Failed: The Directory already existed!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
				return;
			}
			CreateNewDir(inode, order.od[1]);
		}
	}
	string tmps;
	tmps = "  Md order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();

	Writer2();
	return;
}
void Rd() {//rd path
	Writer1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	if (inode == INF) {
		string tmps;
		tmps = "  Failed: The path is not exist!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	else {
		unsigned int tmp = inodetb.inode[inode].last_pos;
		RemoveDir(inode);
		string tmps;
		tmps = "  Rd order executed successfully!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
		CurrentPath = tmp;
	}

	Writer2();
	return;
}
void Newfile() {//newfile filename 或 newfile filename path
	Writer1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	if (order.cnt == 2) {//newfile filename 在当前目录下新建文件
		if (Exist(CurrentPath, order.od[1]) == true) {
			string tmps;
			tmps = "  Failed: The File already existed!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
			return;
		}
		CreateNewFile(CurrentPath, order.od[1]);
	}
	else {//newfile filename path 在path所在的目录下新建目录
		FindAbsolutePath(order.od[2]);
		unsigned int inode = FindFileINode(order.od[2]);
		if (inode == INF) {
			string tmps;
			tmps = "  Failed: The path is not exist!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
		}
		else {
			if (Exist(inode, order.od[1]) == true) {
				string tmps;
				tmps = "  Failed: The File already existed!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
				return;
			}
			CreateNewFile(inode, order.od[1]);
		}
	}
	string tmps;
	tmps = "  Newfile order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();

	Writer2();
	return;
}
void Cat() {//cat path <r, w>
	Reader1();
	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	Reader2();

	if (inode == INF) {
		string tmps;
		tmps = "  Failed: The path is not exist!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	else {
		if (order.od[2] == "r") {
			Reader1();
			CatRead(inode);
			CurrentPath = inodetb.inode[inode].last_pos;
			Reader2();
		}
		else if (order.od[2] == "w") {
			Writer1();
			CatWrite(inode);
			CurrentPath = inodetb.inode[inode].last_pos;
			Writer2();
		}
	}

	return;
}
void Copy() {//copy<host> D:\xxx\yyy\zzz /aaa/bbb <0,1> 或 copy<lxfs> /xxx/yyy/zzz /aaa/bbb
	Writer1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	FindAbsolutePath(order.od[2]);
	if (order.od[0] == "copy<host>") {//host-->Linux
		if (order.od[3] == "0") {//主机拷贝到文件系统
			unsigned int lastpos = order.od[1].find_last_of("\\");//提取文件名
			string filename = order.od[1].substr(lastpos + 1);
			unsigned int inode = FindFileINode(order.od[2]);
			if (inode == INF) {
				string tmps;
				tmps = "  Failed: The path is not exist!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
			}
			else {
				if (Exist(inode, filename) == true) {
					string tmps;
					tmps = "  Failed: The File already existed!\n";
					smo.cnt = 1;
					strcpy_s(smo.str[0], tmps.c_str());
					WriteShareMemory();
					return;
				}

				CopyHost(filename, order.od[1], inode);
			}
		}
		else {//文件系统拷贝到主机
			unsigned int inode = FindFileINode(order.od[2]);
			if (inode == INF) {
				string tmps;
				tmps = "  Failed: The path is not exist!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
			}
			else {
				CatReadToHost(inode, order.od[1]);
			}
		}

	}
	else {//Linux-->Linux
		unsigned int lastpos = order.od[1].find_last_of("/");//提取文件名
		string filename = order.od[1].substr(lastpos + 1);
		FindAbsolutePath(order.od[1]);
		unsigned int inode1 = FindFileINode(order.od[1]);
		unsigned int inode2 = FindFileINode(order.od[2]);
		if (inode1 == INF || inode2 == INF) {
			string tmps;
			tmps = "  Failed: The path is not exist!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps.c_str());
			WriteShareMemory();
		}
		else {
			if (Exist(inode2, filename) == true) {
				string tmps;
				tmps = "  Failed: The File already existed!\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
				return;
			}
			CopyLxfs(filename, inode1, inode2);
		}
	}

	Writer2();
	return;
}
void Del() {//del path
	Writer1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	if (inode == INF) {
		string tmps;
		tmps = "  Failed: The path is not exist!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
	}
	else {
		unsigned int tmp = inodetb.inode[inode].last_pos;
		RemoveFile(inode);
		string tmps;
		tmps = "  Del order executed successfully!\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps.c_str());
		WriteShareMemory();
		CurrentPath = tmp;
	}

	Writer2();
	return;
}
void Check() {//check
	Writer1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	WriteFileSystem();
	string tmps;
	tmps = "  Check order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps.c_str());
	WriteShareMemory();

	Writer2();
	return;
}
void Ls() {//ls
	Reader1();

	ReadFileSystem();//当前进程存储在内存中的数据可能与系统不一致（其他进程可能对文件增删改）
	ShowList();

	Reader2();
	return;
}

void GetUser() {
	hMapFileUser = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		NameUser);
	if (hMapFileUser == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，获取里面的数据
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

	//获得用户名
	char tmpc[50] = "GET";
	while (true) {
		string tmps = string(pBufUser->name);
		if (tmps != "NONE") {
			strcat_s(NameIn, pBufUser->name);
			strcat_s(NameOut, pBufUser->name);
			strcat_s(NameIoo, pBufUser->name);
			strcpy_s(CurrentUser, pBufUser->name);
			Sleep(100);//等待0.1s, 让shell端计算出NameIn、NameOut和NameIoo
			for (int i = 0; i < 50; i++)
				pBufUser->name[i] = tmpc[i];

			break;
		}
		else {//必须要等待一段时间，否则shell端输入的流只会读取到第一个字符改变就进入if
			Sleep(50);
		}
	}

	//关闭
	UnmapViewOfFile(pBufUser);
	CloseHandle(hMapFileUser);

	Sleep(10);
	return;
}

void InitRW() {
	hMapFileRW = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE,
		NameRW);
	if (hMapFileRW == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return;
	}

	pBufRW = (ReaderWriter*)MapViewOfFile(hMapFileRW,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufRW == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileRW);
		return;
	}

	if (pBufRW->first == true) return;


	pBufRW->first = true;

	pBufRW->rw = 1;
	pBufRW->mutex = 1;
	pBufRW->count = 0;
	
	for (int i = 0; i < 50; i++) {
		pBufRW->wakeup1[i] = '\0';
		pBufRW->wakeup2[i] = '\0';
	}
	pBufRW->cnt1 = 0; pBufRW->cnt2 = 0;
	for (int i = 0; i < 30; i++) {
		for (int j = 0; j < 50; j++) {
			pBufRW->wait1[i][j] = '\0';
			pBufRW->wait2[i][j] = '\0';
		}
	}

	return;
}

void Prw() {
	if (pBufRW->rw > 0) {//信号量有效，可以继续执行
		pBufRW->rw = pBufRW->rw - 1;
	}
	else {//信号量无效
		//此进程进入等待列表
		for (int i = 0; i < 50; i++) {
			pBufRW->wait1[pBufRW->cnt1][i] = CurrentUser[i];
		}
		pBufRW->cnt1++;

		//等待唤醒
		while (true) {
			if (strcmp(pBufRW->wakeup1, CurrentUser) == 0) {//唤醒
				pBufRW->rw = pBufRW->rw - 1;
				for (int i = 0; i < 50; i++) {
					pBufRW->wakeup1[i] = '\0';
				}
				return;
			}
			else {
				Sleep(1000);
				string tmps;
				tmps = "  waiting...\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
			}
		}
	}

	return;
}
void Vrw() {
	if (pBufRW->cnt1 == 0) {//没有等待的进程
		pBufRW->rw = pBufRW->rw + 1;
	}
	else {//有等待的进程，唤醒一个并更新等待列表
		pBufRW->rw = pBufRW->rw + 1;
		for (int i = 0; i < 50; i++) {
			pBufRW->wakeup1[i] = pBufRW->wait1[0][i];
		}
		pBufRW->cnt1 = pBufRW->cnt1 - 1;
		for (int i = 0; i < pBufRW->cnt1; i++) {
			for (int j = 0; j < 50; j++) {
				pBufRW->wait1[i][j] = pBufRW->wait1[i + 1][j];
			}
		}
	}

	return;
}

void Pmutex() {
	if (pBufRW->mutex > 0) {//信号量有效，可以继续执行
		pBufRW->mutex = pBufRW->mutex - 1;
	}
	else {//信号量无效
		//此进程进入等待列表
		for (int i = 0; i < 50; i++) {
			pBufRW->wait2[pBufRW->cnt2][i] = CurrentUser[i];
		}
		pBufRW->cnt2++;

		//等待唤醒
		while (true) {
			if (strcmp(pBufRW->wakeup2, CurrentUser) == 0) {//唤醒
				pBufRW->mutex = pBufRW->mutex - 1;
				for (int i = 0; i < 50; i++) {
					pBufRW->wakeup2[i] = '\0';
				}
				return;
			}
			else {
				Sleep(1000);
				string tmps;
				tmps = "  waiting...\n";
				smo.cnt = 1;
				strcpy_s(smo.str[0], tmps.c_str());
				WriteShareMemory();
			}
		}
	}
}
void Vmutex() {
	if (pBufRW->cnt2 == 0) {//没有等待的进程
		pBufRW->mutex = pBufRW->mutex + 1;
	}
	else {//有等待的进程，唤醒一个并更新等待列表
		pBufRW->mutex = pBufRW->mutex + 1;
		for (int i = 0; i < 50; i++) {
			pBufRW->wakeup2[i] = pBufRW->wait2[0][i];
		}
		pBufRW->cnt2 = pBufRW->cnt2 - 1;
		for (int i = 0; i < pBufRW->cnt2; i++) {
			for (int j = 0; j < 50; j++) {
				pBufRW->wait2[i][j] = pBufRW->wait2[i + 1][j];
			}
		}
	}

	return;
}


void Writer1() {
	Prw();
	return;
}
void Writer2() {
	Vrw();
	return;
}


void Reader1() {
	Pmutex();
	if (pBufRW->count == 0)
		Prw();
	pBufRW->count = pBufRW->count + 1;
	Vmutex();

	return;
}
void Reader2() {
	Pmutex();
	pBufRW->count = pBufRW->count - 1;
	if (pBufRW->count == 0)
		Vrw();
	Vmutex();

	return;
}

void Run() {//运行程序
	//step1: 显示个人信息
	string tmps[20];
	tmps[0] = "++++++++++++++++++++++++++++++++++++++++++++++\n";
	tmps[1] = "++              OS Course Design            ++\n";
	tmps[2] = "++        Chen Xingchen    201736612486     ++\n";
	tmps[3] = "++            Teacher       Liu Fagui       ++\n";
	tmps[4] = "++++++++++++++++++++++++++++++++++++++++++++++\n";
	smo.cnt = 5;
	for (int i = 0; i < 5; i++) {
		strcpy_s(smo.str[i], tmps[i].c_str());
	}
	WriteShareMemory();

	//step2: 查看有没有建立过的文件系统，若没有给出初始化选择，若有则进入系统
	bool check = FindDisk();
	if (check == false) {
		tmps[0] = "  Can not find Linux FileSystem, do you need initialization?  Y/N\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps[0].c_str());
		WriteShareMemory();

		string s;
		ReadShareMemory();
		s = smi.str[0];

		if (s == "y" || s == "Y" || s == "yes" || s == "YES" || s == "Yes") {
			CreateFileSystem();
			tmps[0] = "  Init order executed successfully!\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
		}
		else {
			tmps[0] = "  The program will terminate after 2 seconds...\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
			return;
		}
	}
	else {
		tmps[0] = "  Entering the FileSystem...\n";
		smo.cnt = 1;
		strcpy_s(smo.str[0], tmps[0].c_str());
		WriteShareMemory();
		ReadFileSystem();
	}

	//step3: 清空屏幕 给出命令提示 设置当前路径为根目录
	ShowHelp();
	CurrentPath = RootDir;

	//step4: 循环接收命令，直到输入exit
	while (true) {
		ShowPath(); 
		FindOrder(order);//获得命令
		
		switch (order.type) {
		case 0:		//Exit
			tmps[0] = "  Exiting the FileSystem...\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
			return;

		case 1:		//info
			Info();
			break;
		case 2:		//cd path
			Cd();
			break;
		case 3:		//dir 或 dir s 或 dir path 或 dir path s
			Dir();
			break;
		case 4:     //md dirname 或 md dirname path
			Md();
			break;
		case 5:     //rd path
			Rd();
			break;
		case 6:     //newfile filename 或 newfile filename path
			Newfile();
			break;
		case 7:
			Cat();
			break;
		case 8:     //copy<host> D:\xxx\yyy\zzz /aaa/bbb 或 copy<lxfs> /xxx/yyy/zzz /aaa/bbb
			Copy();
			break;
		case 9:     //del path
			Del();
			break;
		case 10:    //check
			Check();
			break;
		case 11:
			Ls();
			break;

		default:
			tmps[0] = "  The code is wrong, please re-enter the order again!...\n";
			smo.cnt = 1;
			strcpy_s(smo.str[0], tmps[0].c_str());
			WriteShareMemory();
		}
	}

	tmps[0] = "  Exit order executed successfully!\n";
	smo.cnt = 1;
	strcpy_s(smo.str[0], tmps[0].c_str());
	WriteShareMemory();

	return;
}

int main() {
	srand((unsigned int)time(NULL));
	
	/*
	RandMd();
	RandNewfile();
	RandCopyhost();
	RandCopylinux();
	Delete();
	RandMd();
	RandNewfile();
	RandCopyhost();
	RandCopylinux();
	Print();
	*/


	//建立读者写者共享内存
	InitRW();

	//与Shell建立连接
	GetUser();

	//创建共享文件
	hMapFileIoo = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		BUF_SIZE,
		NameIoo);
	if (hMapFileIoo == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not create file mapping object (%d).\n"), error);
		return 0;
	}

	// 映射对象的一个视图，得到指向共享内存的指针，设置里面的数据
	pBufIoo = (InputOrOutput*)MapViewOfFile(hMapFileIoo,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		BUF_SIZE);
	if (pBufIoo == NULL) {
		int error = GetLastError();
		_tprintf(TEXT("Could not map view of file (%d).\n"), error);
		CloseHandle(hMapFileIoo);
		return 0;
	}

	//运行程序
	Run();

	//卸载内存映射文件地址指针
	UnmapViewOfFile(pBufIoo);
	//关闭内存映射文件
	CloseHandle(hMapFileIoo);

	return 0;
}