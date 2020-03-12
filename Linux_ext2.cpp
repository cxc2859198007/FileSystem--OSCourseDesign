#include"Linux_ext2.h"
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
	while (cin >> ord.od[ord.cnt++]) {
		if (cin.get() == '\n') break;
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
	else if (order.od[0] == "copy<host>"|| order.od[0] == "copy<lxfs>")          order.type = 8;
	else if (order.od[0] == "del")                                               order.type = 9;
	else if (order.od[0] == "check")                                             order.type = 10;
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
	int cnt = 0;//用户控制换行
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
				if (cnt == 0) cout << "  ";
				cout << c1 << c2 << c3 << c4;
				cnt = cnt + 1;
				if (cnt % 20 == 0) {
					cout << endl;
					cout << "  ";
				}
			}
		}

		//step2: 考虑大文件可能占有多个iNode
		if (inodetb.inode[nowinode].next_pos == INF) break;
		nowinode = inodetb.inode[nowinode].next_pos;
	}
	cout << endl;
	return;
}
void CatWrite(unsigned int nowinode) {
	//step1: 读入一行字符串
	string str;
	cout << "  ";
	getline(cin, str);
	
	//step2: 判断iNode或者数据块够不够
	unsigned int Size = sizeof(str);
	unsigned int needblock = 0, needinode = 0;
	needblock = Size / sizeof(Block);
	if (Size % sizeof(Block) != 0) needblock++;
	needinode = needblock / 17;
	if (needblock % 17 != 0) needinode++;
	
	if (superblock.use_datablock + needblock > superblock.tot_datablock) {
		cout << "  数据块数量不足，拷贝失败！" << endl;
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {
		cout << "  iNode数量不足，拷贝失败！" << endl;
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
	cout << "  写入成功！" << endl;
	
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

	if (superblock.use_datablock + needblock > superblock.tot_datablock) {
		cout << "  数据块数量不足，拷贝失败！" << endl;
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {
		cout << "  iNode数量不足，拷贝失败！" << endl;
	}
	
	//step3: 在dirinode下新建一个文件
	unsigned int nowinode = CreateNewFile(dirinode, filename);

	//step4: 将缓冲区的块放入文件系统
	CopyBufferToLinux(nowinode);
	cout << "  拷贝成功！" << endl;

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

	if (superblock.use_datablock + needblock > superblock.tot_datablock) {
		cout << "  数据块数量不足，拷贝失败！" << endl;
	}
	if (superblock.use_inode + needinode > superblock.tot_inode) {
		cout << "  iNode数量不足，拷贝失败！" << endl;
	}

	//step2: 在dirinode下新建一个文件
	nowinode = CreateNewFile(dirinode, filename);

	//step3: 复制
	 CopyLinuxToLinux(fileinode, nowinode);
	 cout << "  拷贝成功！" << endl;

	//step4: 更新文件系统数据
	WriteFileSystem();
	
	return;
}


void ChangeDir(string newpath) {//改变当前工作目录，newpath是绝对路径
	unsigned int x = FindFileINode(newpath);
	if (x != INF) {
		CurrentPath = x;
		cout << "  当前目录更改成功！" << endl;
	}
	else {
		cout << "  当前目录更改失败！" << endl;
	}
}
bool Exist(unsigned int fathinode, string sonname) {//判断在iNode号为fathinode的目录下有没有名为sonname的文件
	//step1: 直接调用函数找iNode，找不到就是不存在
	unsigned int x = FindFileINode(fathinode, sonname);
	if (x != INF) return true;
	return false;
}


void ShowHelp() {//命令提示
	cout << "************************Linux ext2文件系统************************" << endl << endl;
	cout << "    info                                       显示整个系统信息" << endl;
	cout << "    cd path                                    改变目录" << endl;
	cout << "    dir <path> <s>                             显示目录" << endl;
	cout << "    md dirname <path>                          创建目录" << endl;
	cout << "    rd path                                    删除目录" << endl;
	cout << "    newfile filename <path>                    建立文件" << endl;
	cout << "    cat path <r,w>                             打开文件" << endl;
	cout << "    copy<host> D:\\xxx\\yyy\\zzz /aaa/bbb         拷贝主机文件" << endl;
	cout << "    copy<lxfs> /xxx/yyy/zzz /aaa/bbb           拷贝内部文件" << endl;
	cout << "    del path                                   删除文件" << endl;
	cout << "    check                                      检测并恢复文件系统" << endl;
	cout << endl;

	return;
}
void ShowInfo() {//显示整个系统信息
	cout << "************************Linux ext2文件系统************************" << endl << endl;
	cout << "文件系统结构信息：" << endl;
	cout << "    总磁盘大小：               " << "102400KB  100MB" << endl;
	cout << "    磁盘块大小：               " << superblock.block_size << "B" << endl;
	cout << "    i节点大小：                " << superblock.inode_size << "B" << endl;
	cout << "    磁盘分组个数：             " << "1" << endl;
	cout << "    总磁盘块数：               " << superblock.tot_block << endl;
	cout << "    引导块占用的磁盘块数：     " << groupdes.super_begin << endl;
	cout << "    超级块占用的磁盘块数：     " << groupdes.super_end - groupdes.super_begin + 1 << endl;
	cout << "    组描述符占用的磁盘块数：   " << groupdes.groupdes_end - groupdes.groupdes_begin + 1 << endl;
	cout << "    块位图占用的磁盘块数：     " << groupdes.blockbm_end - groupdes.blockbm_begin + 1 << endl;
	cout << "    i节点位图占用的磁盘块数：  " << groupdes.inodebm_end - groupdes.inodebm_begin + 1 << endl;
	cout << "    i节点表占用的磁盘块数：    " << groupdes.inodetb_end - groupdes.inodetb_begin + 1 << endl;
	cout << "    数据块占用的磁盘块数：     " << groupdes.data_end - groupdes.data_begin + 1 << endl;
	cout << endl;

	cout << "文件系统使用信息：         " << endl;
	cout << "    已使用iNode个数：          " << superblock.use_inode << endl;
	cout << "    已使用数据块个数：         " << superblock.use_datablock <<  endl;
	cout << endl;

	return;
}
void ShowDir(unsigned int nowdir) {//显示iNode号为nowdir的目录信息
	cout << "    目录名：    " << inodetb.inode[nowdir].name << endl;
	cout << "    物理地址：  " << groupdes.data_begin + inodetb.inode[nowdir].block_pos[0] << endl;
	cout << "    文件长度：  " << "128B + 1024B = 1152B" << endl;
	cout << endl;

	CurrentPath = nowdir;

	return;
}
void ShowDir(unsigned int nowdir, bool sonfile) {//显示iNode为nowdir的目录信息，包括子目录名
	cout << "    目录名：          " << inodetb.inode[nowdir].name << endl;
	cout << "    物理地址：        " << groupdes.data_begin + inodetb.inode[nowdir].block_pos[0] << endl;
	cout << "    文件长度：        " << "128B + 1024B = 1152B" << endl;

	unsigned int blockpos = inodetb.inode[nowdir].block_pos[0];
	blockpos = groupdes.data_begin + blockpos;
	Block db;
	ReadBlock(blockpos, db);

	cout << "    共有子目录或子文件个数：  " << inodetb.inode[nowdir].files_num << endl;
	cout << "    子目录的目录名：  ";
	for (int i = 0; i < inodetb.inode[nowdir].files_num; i++) {
		unsigned int nowfile = db.data[i];
		if (inodetb.inode[nowfile].type == 0) {
			cout << inodetb.inode[nowfile].name << " <DIR>   ";
		}
		else {
			cout << inodetb.inode[nowfile].name << " <FILE>   ";
		}
	}
	cout << endl << endl;

	CurrentPath = nowdir;

	return;
}
void CoutPath() {
	string str = inodetb.inode[CurrentPath].name;
	cout << "  " << str << ">$ ";
}


void Info() {//info
	ShowInfo();
	return;
}
void Cd() {//cd path
	FindAbsolutePath(order.od[1]);
	ChangeDir(order.od[1]);
	return;
}
void Dir() {//dir 或 dir s 或 dir path 或 dir path s
	if (order.cnt == 1) {//当前目录，不显示子目录
		ShowDir(CurrentPath);
	}
	else if (order.cnt == 2) {
		if (order.od[1] == "s")//当前目录，显示子目录
			ShowDir(CurrentPath, true);
		else {//指定目录，不显示子目录
			FindAbsolutePath(order.od[1]);
			unsigned int inode = FindFileINode(order.od[1]);
			if (inode == INF) cout << "  输入路径不存在，查看失败！" << endl;
			else ShowDir(inode);
		}
	}
	else if (order.cnt == 3) {//指定目录，显示子目录
		FindAbsolutePath(order.od[1]);
		unsigned int inode = FindFileINode(order.od[1]);
		if (inode == INF) cout << "  输入路径不存在，查看失败！" << endl;
		else ShowDir(inode, true);
	}
	return;
}
void Md() {//md dirname 或 md dirname path
	if (order.cnt == 2) {//md dirname 在当前目录下新建目录
		if (Exist(CurrentPath, order.od[1]) == true) {
			cout << "  该目录已存在，新建失败！" << endl;
			return;
		}
		CreateNewDir(CurrentPath, order.od[1]);
	}
	else {//md dirname path 在path所在的目录下新建目录
		FindAbsolutePath(order.od[2]);
		unsigned int inode = FindFileINode(order.od[2]);
		if (inode == INF) cout << "  输入路径不存在，新建失败！" << endl;
		else {
			if (Exist(inode, order.od[1]) == true) {
				cout << "  该目录已存在，新建失败！" << endl;
				return;
			}
			CreateNewDir(inode, order.od[1]);
		}
	}
	cout << "  新目录创建成功！" << endl;

	return;
}
void Rd() {//rd path
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	if (inode == INF) cout << "  输入路径不存在，删除失败！" << endl;
	else {
		unsigned int tmp = inodetb.inode[inode].last_pos;
		RemoveDir(inode);
		cout << "  目录删除成功！" << endl;
		CurrentPath = tmp;
	}
	return;
}
void Newfile() {//newfile filename 或 newfile filename path
	if (order.cnt == 2) {//newfile filename 在当前目录下新建文件
		if (Exist(CurrentPath, order.od[1]) == true) {
			cout << "  该文件已存在，新建失败！" << endl;
			return;
		}
		CreateNewFile(CurrentPath, order.od[1]);
	}
	else {//newfile filename path 在path所在的目录下新建目录
		FindAbsolutePath(order.od[2]);
		unsigned int inode = FindFileINode(order.od[2]);
		if (inode == INF) cout << "  输入路径不存在，新建失败！" << endl;
		else {
			if (Exist(inode, order.od[1]) == true) {
				cout << "  该文件已存在，新建失败！" << endl;
				return;
			}
			CreateNewFile(inode, order.od[1]);
		}
	}
	cout << "  新文件创建成功！" << endl;
	return;
}
void Cat() {//cat path <r, w>
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	if (inode == INF) cout << "  输入路径不存在，打开失败！" << endl;
	else {
		if (order.od[2] == "r") {
			CatRead(inode);
		}
		else if (order.od[2] == "w") {
			CatWrite(inode);
		}
		CurrentPath = inodetb.inode[inode].last_pos;
	}

	return;
}
void Copy() {//copy<host> D:\xxx\yyy\zzz /aaa/bbb 或 copy<lxfs> /xxx/yyy/zzz /aaa/bbb
	FindAbsolutePath(order.od[2]);

	if (order.od[0] == "copy<host>") {//host-->Linux
		unsigned int lastpos = order.od[1].find_last_of("\\");//提取文件名
		string filename = order.od[1].substr(lastpos + 1);
		unsigned int inode = FindFileINode(order.od[2]);
		if (inode == INF) cout << "  输入路径不存在，拷贝失败！" << endl;
		else {
			if (Exist(inode, filename) == true) {
				cout << "  该文件已存在，拷贝失败！" << endl;
				return;
			}
			
			CopyHost(filename, order.od[1], inode);
		}
	}
	else {//Linux-->Linux
		unsigned int lastpos = order.od[1].find_last_of("/");//提取文件名
		string filename = order.od[1].substr(lastpos + 1);
		FindAbsolutePath(order.od[1]);
		unsigned int inode1 = FindFileINode(order.od[1]);
		unsigned int inode2 = FindFileINode(order.od[2]);
		if (inode1 == INF || inode2 == INF) cout << "  输入路径不存在，拷贝失败！" << endl;
		else {
			if (Exist(inode2, filename) == true) {
				cout << "  该文件已存在，拷贝失败！" << endl;
				return;
			}
			CopyLxfs(filename, inode1, inode2);
		}
	}
	return;
}
void Del() {//del path
	FindAbsolutePath(order.od[1]);
	unsigned int inode = FindFileINode(order.od[1]);
	if (inode == INF) cout << "  输入路径不存在，删除失败！" << endl;
	else {
		unsigned int tmp = inodetb.inode[inode].last_pos;
		RemoveFile(inode);
		cout << "  文件删除成功！" << endl;
		CurrentPath = tmp;
	}

	return;
}
void Check() {//check
	WriteFileSystem();
	cout << "  已将内存中的文件系统信息写入磁盘！" << endl;
	cout << "  已对文件系统进行数据在整理！" << endl;

	return;
}
void Run() {//运行程序
	//step1: 显示个人信息
	cout << "++++++++++++++++++++++++++++++++++++++" << endl;
	cout << "++         操作系统课程设计         ++" << endl;
	cout << "++       陈星辰    201736612486     ++" << endl;
	cout << "++         指导老师： 刘发贵        ++" << endl;
	cout << "++++++++++++++++++++++++++++++++++++++" << endl;
	
	//step2: 查看有没有建立过的文件系统，若没有给出初始化选择，若有则进入系统
	bool check = FindDisk();
	if (check == false) {
		cout << "  检测到没有Linux文件系统，是否初始化？Y/N" << endl;
		string s; cin >> s;
		if (s == "y" || s == "Y" || s == "yes" || s == "YES" || s == "Yes") {
			cout << "  正在初始化，请稍后..." << endl;
			CreateFileSystem();
			cout << "  创建成功，正在进入系统" << endl;
			Sleep(300);
		}
		else {
			cout << "  没有文件系统，两秒后结束程序" << endl;
			Sleep(2000);
			return;
		}
	}
	else {
		cout << "  正在进入系统..." << endl;
		ReadFileSystem();
		Sleep(500);
	}

	//step3: 清空屏幕 给出命令提示 设置当前路径为根目录
	system("cls");
	ShowHelp();
	CurrentPath = RootDir;

	//step4: 循环接收命令，直到输入exit
	while (1) {
		CoutPath(); FindOrder(order);//获得命令
		
		switch (order.type) {
		case 0:		//Exit
			cout << "  正在退出系统..." << endl;
			Sleep(500);
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

		default:
			cout << "  输入指令有误，请重新输入！" << endl;
		}
	}

	return;
}


int main() {
	Run();
	cout << "  已退出系统！" << endl;
	return 0;
}