#include"head.h"

void init() {
	fstream fd(DISK_NAME, ios::out);//清空
	fd.close();
	fd.open(DISK_NAME, ios::out | ios::in | ios::binary);

	//如果没有打开文件则输出提示信息并throw错误
	if (!fd.is_open()) {
		cout << "无法打开文件卷" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}
	//cout << "success!" << endl;

	//SuperBlock init
	//SuperBlock superblock;
	superblock.s_isize = (FILE_POS - 1 - INODE_POS + 1);	/* 外存Inode区占用的盘块数 2# - 59# mention innode number should * 8 */
	superblock.s_fsize = IMG_SIZE * 2 * 1024 - 1 - FILE_POS + 1;	/* file部分盘块总数 60# - 128*2*1024-1 # */
	//superblock.s_fsize = IMG_SIZE * 2 - 1 - FILE_POS + 1;	/* 测试 */
	superblock.s_nfree = FREE_INDEX_NUM;				/* 直接管理的空闲盘块数量 */
	for (int i = 0; i < FREE_INDEX_NUM; i++) {
		superblock.s_free[i] = FREE_INDEX_NUM - i - 1;
	}
	//此时s_free[0] = 99,但是我们要让s_free[0]指向下一个链接的地址，所以要改掉
	superblock.s_free[0] = 1; //FILE部分第二个block用来装s-free的下一个链接
	//s_nfree在链接后更改

	superblock.s_ninode = FREE_INDEX_NUM - 1;				/* 直接管理的空闲外存Inode数量 */
	for (int i = 0; i < FREE_INDEX_NUM; i++) {
		superblock.s_inode[i] = FREE_INDEX_NUM - i - 1;
	}
	//开始成组链接
	int used_block = 0;
	int block_pos = FREE_INDEX_NUM - 1;
	//链接file部分
	unsigned int stack[FREE_INDEX_NUM];
	int i_max = superblock.s_fsize / (FREE_INDEX_NUM - 1) + (superblock.s_fsize % (FREE_INDEX_NUM - 1) != 0);
	for (int i = 2; i <= i_max; i++) {
		memset(stack, 0, FREE_INDEX_NUM * sizeof(int));
		used_block++;
		int nfree = (superblock.s_fsize - (i - 1) * (FREE_INDEX_NUM - 1)) > (FREE_INDEX_NUM - 1) ? (FREE_INDEX_NUM - 1) : (superblock.s_fsize - (i - 1) * (FREE_INDEX_NUM - 1));
		for (int j = nfree; j > 0; j--) {
		//for (unsigned j = 0; j < FREE_INDEX_NUM && ((i - 1) * (FREE_INDEX_NUM - 1) + j) < superblock.s_fsize; j++) {
			//stack[j] = (FREE_INDEX_NUM - 1) * i - j;
			stack[j] = block_pos++;
		}
		if (i == i_max)
			stack[0] = 0;
		else {
			stack[0] = i;
		}
		//写入内存
		if (i != i_max)
			fd.seekg((FILE_POS + stack[0] - 1) * BLOCK_SIZE, ios::beg);
		else
			fd.seekg((FILE_POS + i_max - 1) * BLOCK_SIZE, ios::beg);
		fd.write((char*)&nfree, sizeof(int));
		fd.write((char*)&stack, FREE_INDEX_NUM * sizeof(int));
	}
	//链接innode部分
	block_pos = FREE_INDEX_NUM - 1;
	superblock.s_inode[0] = i_max ;//innode下一链接位置
	int i_max2 = superblock.s_isize * 8 / (FREE_INDEX_NUM - 1) + (superblock.s_isize * 8 % (FREE_INDEX_NUM - 1) != 0);
	for (int i = 2; i <= i_max2; i++) {
		memset(stack, 0, FREE_INDEX_NUM * sizeof(int));
		used_block++;
		int nfree = (superblock.s_isize * 8 - (i - 1) * (FREE_INDEX_NUM - 1)) > (FREE_INDEX_NUM - 1) ? (FREE_INDEX_NUM - 1) : (superblock.s_isize * 8 - (i - 1) * (FREE_INDEX_NUM - 1));
		for(int j = nfree; j > 0;j-- ){
		//for (unsigned j = 0; j < FREE_INDEX_NUM && ((i - 1) * (FREE_INDEX_NUM - 1) + j) < superblock.s_isize; j++) {
			//stack[j] = (FREE_INDEX_NUM - 1) * i - j;
			stack[j] = block_pos++;
		}
		if (i == i_max2)
			stack[0] = 0;
		else {
			stack[0] = superblock.s_inode[0] + i - 1;
		}
		//写入内存
		if (i != i_max2)
			fd.seekg((FILE_POS + stack[0] - 1) * BLOCK_SIZE, ios::beg);
		else
			fd.seekg((FILE_POS + superblock.s_inode[0] + i_max2 - 2) * BLOCK_SIZE, ios::beg);
		fd.write((char*)&nfree, sizeof(int));
		fd.write((char*)&stack, FREE_INDEX_NUM * sizeof(int));
	}
	used_block++;//用了的block数
	if (used_block > 99) {//要删除链接块的空闲块
		used_block -= 99;
		superblock.s_nfree = 0;
		memset(superblock.s_free + 1, BLOCK_USED, (FREE_INDEX_NUM - 1) * sizeof(int));
		//下面删链接块
		int next_free = superblock.s_free[0];//下一链接块的地址
		int stack[FREE_INDEX_NUM + 2];
		int nfree;
	START:
		getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
		nfree = stack[0];
		stack[FREE_INDEX_NUM + 1] = '\0';
		for (int i = nfree + 1; i > 1 && used_block > 0 ; i--) {
			stack[i] = BLOCK_USED;
			used_block--;
		}
		//删除玩的写入cache
		cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//将原来的页变成空白页
		if (used_block > 0) {//接着删除下一个链接块
			next_free = stack[1];
			goto START;
		}
	}
	else {
		superblock.s_nfree -= used_block;
		//用了useblock+1块
		for (int i = 0; i < used_block; i++) {
			superblock.s_free[FREE_INDEX_NUM - i - 1] = BLOCK_USED;
		}
	}
	//到此superblock初始化完毕，但是还没写进内存

	//创建根目录inode
	DiskInode Inode_root;
	Inode_root.d_mode = DiskInode::DIRECTORY;//目录
	//Inode_root.d_nlink = 0;//联结计数
	//Inode_root.d_uid = 0;//管理员
	//Inode_root.d_gid = 1;//文件所有者的组标识
	Inode_root.f_flag = 0;//不是文件
	Inode_root.f_offset = 0;
	Inode_root.d_size = 0;//目录大小为0
	Inode_root.d_addr[0] = 0;//根目录对应Block
	Inode_root.d_atime = time(NULL);//最后访问时间
	//Inode_root.d_mtime = time(NULL);//最后访问时间
	//写入内存
	fd.seekg(INODE_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&Inode_root, sizeof(Inode_root));
	//根目录block写入0号block
	Directory root_directory;
	strcpy(root_directory.name, "root");
	strcpy(root_directory.d_filename[0], ".");//0是自己
	root_directory.d_inodenumber[0] = 0;
	strcpy(root_directory.d_filename[1], "..");//1是父亲（此处还是自己）
	root_directory.d_inodenumber[1] = 0;
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		root_directory.d_filename[i][0] = '\0';
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		root_directory.d_inodenumber[i] = -1;
	}
	fd.seekg(FILE_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&root_directory, sizeof(root_directory));
	//至此file部分，inode部分初始化完成
	//0号innode用了
	//superblock.s_nfree -= 1;
	superblock.s_ninode -= 1;
	superblock.s_inode[FREE_INDEX_NUM - 1] = BLOCK_USED;
	fd.seekg(SUPERBLOCK_POS* BLOCK_SIZE, ios::beg);
	fd.write((char*)&superblock, sizeof(superblock));


	//进入根目录
	fd.seekg(FILE_POS* BLOCK_SIZE, ios::beg);
	fd.read((char*)&directory, sizeof(directory));
	fd.close();

	//初始化openfile
	for (int i = 0; i < OPEN_FILE_NUM; i++) {
		open_file[i] = -1;
	}

	mkdir("bin");
	mkdir("etc");
	mkdir("home");
	mkdir("dev");
	cd("home");
	mkdir("texts");
	mkdir("reports");
	mkdir("photos");
	cd("..");

	exit();
}

//根据blkno得到内容，在cache中找，找不到就读img,并写到cache中
void getblk(const int blkno, int type, char* content, int length) {
	//cout << "getblk:" << blkno << " " << type << endl;
	CacheBlock* pb = cachelist.findCache(blkno, type);
	//cout << "getblk:pb:" << " " << pb->blkno << " " << pb->type << endl;
	if (pb != NULL) {//cache里面有这个block
		cachelist.readCache(pb, content);
	}
	else {
		fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
		//如果没有打开文件则输出提示信息并throw错误
		if (!fd.is_open()) {
			cout << "无法打开文件卷" << DISK_NAME << endl;
			throw(CANT_OPEN_FILE);
		}
		if (type == CacheBlock::INODE) {
			fd.seekg(INODE_POS * BLOCK_SIZE + (blkno - INODE_POS) * INODE_SIZE, ios::beg);
			fd.read(content, length);
		}
		else {
			fd.seekg((blkno)*BLOCK_SIZE, ios::beg);
			fd.read(content, length);
		}
		//cout << content;
		fd.close();
		cachelist.writeCache(blkno, type, content, length);
	}
}

//创建子目录
void mkdir(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "目录名称非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	//查看当前目录，如果有同名报错
	bool cant_create = true;//目录文件数目达到上限
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {
			cout << "同名文件夹已存在！" << endl;
			return;
		}
		if (directory.d_filename[i][0] == '\0') {
			cant_create = false;
		}
	}
	if (cant_create == true) {
		cout << "目录文件数目已达上限！" << endl;
		return;
	}
	int freeBlock = superblock.findFreeFile();//空闲block序号
	int freeInode = superblock.findFreeInode();//空闲indode序号
	if (freeBlock == NO_ROOM_FOR_FILE) return;
	if (freeInode == NO_ROOM_FOR_FILE) return;
	//创建目录inode
	DiskInode new_Inode;
	new_Inode.d_mode = DiskInode::DIRECTORY;//目录
	new_Inode.f_flag = 0;
	new_Inode.f_offset = 0;
	new_Inode.d_size = 0;//目录大小为0
	new_Inode.d_addr[0] = freeBlock;//目录对应Block
	new_Inode.d_atime = time(NULL);//最后访问时间
	//创建目录
	Directory new_directory;
	strcpy(new_directory.name, name);
	strcpy(new_directory.d_filename[0], ".");//0是自己
	new_directory.d_inodenumber[0] = freeInode;
	strcpy(new_directory.d_filename[1], "..");//1是父亲
	new_directory.d_inodenumber[1] = directory.d_inodenumber[0];//当前目录的inode号是父亲
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		new_directory.d_filename[i][0] = '\0';
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		new_directory.d_inodenumber[i] = -1;
	}
	//修改当前目录
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] == '\0') {
			strcpy(directory.d_filename[i], new_directory.name);
			directory.d_inodenumber[i] = freeInode;
			//这里可能要处理一下名字长短问题
			break;
		}
	}
	//directory是全局变量，不写到cache中，exit的时候直接写入img即可

	//new_dictory写入cache
	cachelist.writeCache(freeBlock + FILE_POS, CacheBlock::BLOCK, (char*)&new_directory, sizeof new_directory);
	//inode写入cache
	cachelist.writeCache(freeInode + INODE_POS, CacheBlock::INODE, (char*)&new_Inode, sizeof new_Inode);
	//完成
	cout << "新建文件夹 "<<name<<" 完成" << endl;
}

//列目录
void ls() {
	cout << "当前目录: " << directory.name << endl;
	cout << "子目录：" << endl;
	cout << "**********************************************" << endl;
	cout << "  文件类型    文件名    最后访问时间" << endl;
	cout << "**********************************************" << endl;
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] != '\0') {
			//cout <<directory.d_inodenumber[i] << " " << directory.d_filename[i] << endl;
			DiskInode inode;//本file的inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
			if (inode.d_mode == DiskInode::DIRECTORY) {
				cout << "  文件夹      ";
			}
			else if (inode.d_mode == DiskInode::FILE) {
				cout << "  文件        ";
			}
			cout << directory.d_filename[i];
			//cout << strlen(directory.d_filename[i]) << " " << 5 - (strlen(directory.d_filename[i]) - 3) << endl;
			for (int j = 0; j < (int)(5 - (strlen(directory.d_filename[i]) - 3)); j++) {
				cout << " ";
			}
			cout << " ";
			cout << ctime(&inode.d_atime) << endl;
		}
	}
	//cout << endl << "directory" << endl;
	//for (int j = 0; j < sizeof directory; j++) {
	//	cout << (int)((char*)&directory)[j] << " ";
	//	if (j % 16 == 15)cout << endl;
	//}
}

void cd(const char* name) {
	if (strcmp(name, "..") == 0) {//回到上层文件夹
		DiskInode parent;//parent的inode
		getblk(directory.d_inodenumber[1] + INODE_POS, CacheBlock::INODE, (char*)&parent, sizeof parent);
		DiskInode old;//现在的inode
		getblk(directory.d_inodenumber[0] + INODE_POS, CacheBlock::INODE, (char*)&old, sizeof old);
		cachelist.writeCache(old.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//现directory写入cache
		getblk(parent.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//directory易主		
		//for (int j = 0; j < sizeof directory; j++) {
		//	cout << (int)((char*)&directory)[j] << " ";
		//	if (j % 16 == 15)cout << endl;
		//}
		return;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {//找到本文件夹
			DiskInode new_inode;//本file的inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&new_inode, sizeof new_inode);
			if (new_inode.d_mode != DiskInode::DIRECTORY) {
				continue;
			}
			DiskInode old;//现directory的inode
			getblk(directory.d_inodenumber[0] + INODE_POS, CacheBlock::INODE, (char*)&old, sizeof old);
			cachelist.writeCache(old.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//现directory写入cache

			getblk(new_inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//directory易主
			
			//cout << endl << "directory" << endl;
			//for (int j = 0; j < sizeof directory; j++) {
			//	cout << (int)((char*)&directory)[j] << " ";
			//	if (j % 16 == 15)cout << endl;
			//}
			
			return;
		}
	}
	cout << "没有文件夹" << name << endl;
	return;
}

//新建文件
void fcreate(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "文件名称非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	//查看当前目录，如果有同名报错
	bool cant_create = true;//目录文件数目达到上限
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {
			cout << "同名文件已存在！" << endl;
			return;
		}
		if (directory.d_filename[i][0] == '\0') {
			cant_create = false;
		}
	}
	if (cant_create == true) {
		cout << "目录文件数目已达上限！" << endl;
		return;
	}
	int freeBlock = superblock.findFreeFile();//空闲block序号
	int freeInode = superblock.findFreeInode();//空闲indode序号
	if (freeBlock == NO_ROOM_FOR_FILE) return;
	if (freeInode == NO_ROOM_FOR_FILE) return;
	//创建目录inode
	DiskInode new_Inode;
	new_Inode.d_mode = DiskInode::FILE;//目录
	//new_Inode.d_nlink = 0;//联结计数
	//new_Inode.d_uid = 0;//管理员
	//new_Inode.d_gid = 1;//文件所有者的组标识
	new_Inode.f_flag = DiskInode::FWRITE;//可读写
	new_Inode.f_offset = 0;
	new_Inode.d_size = 0;//当前文件大小为0
	new_Inode.d_addr[0] = freeBlock;//文件对应Block
	new_Inode.d_atime = time(NULL);//最后访问时间
	//创建文件
	File new_File;
	//memcpy(new_File.content, name, sizeof name);
	//strcpy(new_File.content, name);

	//修改当前目录
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] == '\0') {
			strcpy(directory.d_filename[i], name);//0是自己
			//memcpy(directory.d_filename[i], name, strlen(name));
			//strcpy(directory.d_filename[i], new_File.name);
			directory.d_inodenumber[i] = freeInode;
			//这里可能要处理一下名字长短问题
			break;
		}
	}
	//new_File写入cache
	cachelist.writeCache(freeBlock + FILE_POS, CacheBlock::BLOCK, (char*)&new_File, sizeof new_File);
	//inode写入cache
	cachelist.writeCache(freeInode + INODE_POS, CacheBlock::INODE, (char*)&new_Inode, sizeof new_Inode);
	//完成
	cout << "新建文件 "<<name<<" 完成" << endl;
}

int fopen(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "文件名称非法！" << endl;
		//throw(NAME_LONG);
		return -1;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(name, directory.d_filename[i]) == 0) {//目录下有这个文件
			int pos = 0;
			for (; pos < OPEN_FILE_NUM; pos++) {
				if (open_file[pos] == -1) {//空的pair
					break;
				}
			}
			if (pos == OPEN_FILE_NUM) {
				cout << "打开文件个数已达上限" << endl;
				return OPEN_TOO_MANY_FILES;
			}

			open_file[pos] = directory.d_inodenumber[i];
			cout << "打开文件 " << name << " 成功，文件句柄 " << pos << endl;
			return pos;
		}
	}
	//目录下没有这个文件
	cout << "不存在文件 " << name << endl;
	return -1;
}

//关闭fdnum对应的fstream
void fclose(int fdnum) {
	if(fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "文件句柄非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "不存在文件句柄" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	open_file[fdnum] = -1;
	cachelist.writeBack();//刷一次缓存
	cout << "关闭文件句柄 " << fdnum << " 成功" << endl;
}

//更改文件读写权限
void fflag(int fdnum, string type) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "文件句柄非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "不存在文件句柄" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	DiskInode inode;//本file的inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//此时inode load完成
	if (type == "read") {
		inode.f_flag = inode.FREAD;
		cout << "文件权限已更改为只读！" << endl;
	}
	else if (type == "write") {
		inode.f_flag = inode.FWRITE;
		cout << "文件权限已更改为可写！" << endl;
	}
	else {
		cout << "权限名不正确！" << endl;
		return;
	}
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inode写入cache
	
}

//文件指针移动dis距离
void fseek(int fdnum, int dis, int type) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "文件句柄非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "不存在文件句柄" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	DiskInode inode;//本file的inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//此时inode load完成
	int old = inode.f_offset;
	if (type == BEG) {
		inode.f_offset = dis;
	}
	else if (type == CUR) {
		inode.f_offset += dis;
	}
	else if (type == END) {
		inode.f_offset = inode.d_size + dis;
	}
	if (inode.f_offset < 0) inode.f_offset = 0;
	else if (inode.f_offset > inode.d_size) inode.f_offset = inode.d_size;
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inode写入cache
	cout << "文件指针移动至 " << inode.f_offset << " ,原本文件指针位于 " << old << " ,移动距离 " << dis << endl;
}

//inode：文件inode；int i：此inode对应在directory的位置，用来删除inode的
inline void delete_file(const DiskInode inode, int i) {
	char blank[BLOCK_SIZE] = { '\0' };
	vector<int> reset_blk;
	int stack[INDEX_NUM + 2];
	stack[INDEX_NUM + 1] = '\0';
	int nfree;
	for (int j = 0; j <= inode.d_size; j += BLOCK_SIZE) {
		int offset = j;
		vector<int> addrr = off_to_block(offset);//offset定位到block
		int blkno;
		
		if (addrr[2] != -1) {//存在二级索引
			blkno = inode.d_addr[addrr[2]];
			if(find(reset_blk.begin(), reset_blk.end(), blkno) == reset_blk.end())
				reset_blk.push_back(blkno);
			getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
		}
		if (addrr[1] != -1) {
			if (addrr[2] == -1) blkno = inode.d_addr[addrr[1]];
			else blkno = stack[addrr[1]];
			if (find(reset_blk.begin(), reset_blk.end(), blkno) == reset_blk.end())
				reset_blk.push_back(blkno);
			getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
		}
		if (addrr[0] != -1) {
			if (addrr[2] == -1 && addrr[1] == -1) blkno = inode.d_addr[addrr[0]];
			else blkno = stack[addrr[0]];
			if (find(reset_blk.begin(), reset_blk.end(), blkno) == reset_blk.end())
				reset_blk.push_back(blkno);
		}
	}

	for (auto blkno : reset_blk) {
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//将原来的页变成空白页
		//下面将空闲的block插入回superblock
		if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//要找下一个链接块
			int next_free = superblock.s_free[0];//下一链接块的地址
		START:
			getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
			nfree = stack[0];
			if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
				next_free = stack[1];
				goto START;
			}
			else {
				for (int j = 2; j <= nfree + 1; j++) {//从后往前找第一个不是used的值
					if (stack[j] == BLOCK_USED) {
						stack[j] = blkno;
						break;
					}
				}
				//结束，写入cache
				cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//将原来的页变成空白页
			}
		}
		else {
			superblock.s_free[++superblock.s_nfree] = blkno;
		}
	}

	//清除此inode
	cachelist.writeCache(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, blank, sizeof inode);//inode写入cache
	//将inode对应的页号放回superblock
	if (superblock.s_ninode == FREE_INDEX_NUM) {//要找下一个链接块
		int next_free = superblock.s_inode[0];//下一链接块的地址
		int stack[FREE_INDEX_NUM+2];
		stack[FREE_INDEX_NUM + 1] = '\0';
		int nfree;
		START1:
		getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
		nfree = stack[0];
		if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
			next_free = stack[1];
			goto START1;
		}
		else {
			for (int j = 2; j <= nfree + 1; j++) {//从后往前找第一个不是used的值
				if (stack[j] == BLOCK_USED) {
					stack[j] = directory.d_inodenumber[i];
					break;
				}
			}
			//结束，写入cache
			cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//将原来的页变成空白页
		}
	}
	else
		superblock.s_inode[++superblock.s_ninode] = directory.d_inodenumber[i];

}

inline void delete_directory() {
	char blank[BLOCK_SIZE] = {'\0'};
	int stack[FREE_INDEX_NUM+2];
	stack[FREE_INDEX_NUM + 1] = '\0';
	int nfree;
	for (int j = 2; j < SUBDIRECTORY_NUM; j++) {
		if (directory.d_inodenumber[j] != -1) {//有东西的
			DiskInode mid;//本file的inode
			getblk(directory.d_inodenumber[j] + INODE_POS, CacheBlock::INODE, (char*)&mid, sizeof mid);
			if (mid.d_mode == mid.FILE) {//是文件
				delete_file(mid, j);
			}
			else {//是文件夹
				Directory old;
				memcpy(&old, &directory, sizeof directory);//保存一个副本
				getblk(mid.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//得到本目录
				delete_directory();
				memcpy(&directory, &old, sizeof directory);//回档

				//目录页变为空白页
				cachelist.writeCache(mid.d_addr[0] + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//将原来的页变成空白页
				//下面将空闲的block插入回superblock
				if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//要找下一个链接块
					int next_free = superblock.s_free[0];//下一链接块的地址
				START:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
						next_free = stack[1];
						goto START;
					}
					else {
						for (int j = 2; j <= nfree + 1 ; j++) {//从后往前找第一个不是used的值
							if (stack[j] == BLOCK_USED) {
								stack[j] = mid.d_addr[0];
								break;
							}
						}
						//结束，写入cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//将原来的页变成空白页
					}
				}
				else
					superblock.s_free[++superblock.s_nfree] = mid.d_addr[0];

				//清除文件夹的inode
				cachelist.writeCache(directory.d_inodenumber[j] + INODE_POS, CacheBlock::INODE, blank, sizeof mid);//inode写入cache
				//将inode对应的页号放回superblock
				if (superblock.s_ninode == FREE_INDEX_NUM - 1) {//要找下一个链接块
					int next_free = superblock.s_inode[0];//下一链接块的地址
					int stack[FREE_INDEX_NUM+2];
					stack[FREE_INDEX_NUM + 1] = '\0';
					int nfree;
				START1:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
						next_free = stack[1];
						goto START1;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//从后往前找第一个不是used的值
							if (stack[j] == BLOCK_USED) {
								stack[j] = directory.d_inodenumber[j];
								break;
							}
						}
						//结束，写入cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//将原来的页变成空白页
					}
				}
				else
					superblock.s_inode[++superblock.s_ninode] = directory.d_inodenumber[j];
			}

		}
	}
}

int fdelete(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "文件名称非法！" << endl;
		//throw(NAME_LONG);
		return NAME_LONG;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(name, directory.d_filename[i]) == 0) {//目录下有这个文件
			for (int pos = 0; pos < OPEN_FILE_NUM; pos++) {
				if (open_file[pos] == directory.d_inodenumber[i]) {//本文见已经打开了
					fclose(pos);
					break;
				}
			}
			//更改cache，directory
			DiskInode inode;//本file的inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
			//取得本文见inode
			if (inode.d_mode == inode.FILE) {//是文件
				delete_file(inode, i);
			}
			else {//是文件夹
				Directory old;
				memcpy(&old, &directory, sizeof directory);//保存一个副本
				getblk(inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//得到本目录
				delete_directory();
				memcpy(&directory, &old, sizeof directory);//回档

				char blank[BLOCK_SIZE] = { '\0' };
				int nfree;
				int stack[FREE_INDEX_NUM + 2];
				stack[FREE_INDEX_NUM + 1] = '\0';
				//目录页变为空白页
				cachelist.writeCache(inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//将原来的页变成空白页
				//下面将空闲的block插入回superblock
				if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//要找下一个链接块
					int next_free = superblock.s_free[0];//下一链接块的地址
				START:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
						next_free = stack[1];
						goto START;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//从后往前找第一个不是used的值
							if (stack[j] == BLOCK_USED) {
								stack[j] = inode.d_addr[0];
								break;
							}
						}
						//结束，写入cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//将原来的页变成空白页
					}
				}
				else
					superblock.s_free[++superblock.s_nfree] = inode.d_addr[0];

				//清除文件夹的inode
				cachelist.writeCache(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, blank, sizeof inode);//inode写入cache
				//将inode对应的页号放回superblock
				if (superblock.s_ninode == FREE_INDEX_NUM - 1) {//要找下一个链接块
					int next_free = superblock.s_inode[0];//下一链接块的地址
					int nfree;
					int stack[FREE_INDEX_NUM + 2];
					stack[FREE_INDEX_NUM + 1] = '\0';
				START1:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//再找下一个链接块
						next_free = stack[1];
						goto START1;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//从后往前找第一个不是used的值
							if (stack[j] == BLOCK_USED) {
								stack[j] = directory.d_inodenumber[j];
								break;
							}
						}
						//结束，写入cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//将原来的页变成空白页
					}
				}
				else
					superblock.s_inode[++superblock.s_ninode] = directory.d_inodenumber[i];
			}

			
			//更新目录
			directory.d_inodenumber[i] = -1;
			directory.d_filename[i][0] = '\0';

			cout << "删除文件(夹) " << name << " 成功！" << endl;
			return SUCCESS;
		}
	}
	//no this file
	cout << "不存在文件 " << name << endl;
	return FILE_NOT_EXIST;
}

//根据offset找对应的block,[0]对应直接索引，[1]对应一级索引,[2]对应2级索引(注意是下标)
vector<int> off_to_block(int& offset) {
	vector<int> res(3,0);
	if (offset >= 0 && offset < 6 * BLOCK_SIZE) {
		res[1] = -1; res[2] = -1;//只需要直接索引
		if (offset >= 0 && offset < BLOCK_SIZE) {
			res[0] = 0;
			offset %= BLOCK_SIZE;
		}
		else {
			res[0] = (offset - (BLOCK_SIZE)) / BLOCK_SIZE + 1;
			offset = (offset - (BLOCK_SIZE)) % BLOCK_SIZE;
		}
	}
	else if (offset >= 6 * BLOCK_SIZE && offset < (128 * 2 + 6) * BLOCK_SIZE) {
		res[2] = -1;//要一级索引和直接索引
		res[1] = 6 + (offset - (6 * BLOCK_SIZE)) / (128 * BLOCK_SIZE);
		res[0] = (offset - (6 * BLOCK_SIZE) - (res[1] - 6) * (128 * BLOCK_SIZE)) / BLOCK_SIZE;
		offset = (offset - (BLOCK_SIZE)) % BLOCK_SIZE;
	}
	else if(offset >= (128 *2 + 6) * BLOCK_SIZE && offset < (6 + 128 * 2 + 128 * 128 * 2) * BLOCK_SIZE) {//need all
		res[2] = 8 + (offset - (6 + 128 * 2) * BLOCK_SIZE) / (128 * 128 * BLOCK_SIZE);
		res[1] = (offset - (6 + 128 * 2) * BLOCK_SIZE - (res[2]- 8) * (128 * 128 * BLOCK_SIZE)) / (128 * BLOCK_SIZE);
		res[0] = (offset - (6 + 128 * 2) * BLOCK_SIZE - (res[2]- 8) * (128 * 128 * BLOCK_SIZE) - res[1] * (128 * BLOCK_SIZE)) / (BLOCK_SIZE);
		offset = (offset - (BLOCK_SIZE)) % BLOCK_SIZE;
	}
	else {//offset过大或者过小
		cout << "不合法的offset" << endl;
		throw(ILLEGAL_OFFSET);
	}
	return res;
}

//给file文件分配一个新block，同时更新inode，进cache
void new_block_to_file(DiskInode& inode) {
	int offset = inode.f_offset;
	vector<int> addrr = off_to_block(offset);//offset定位到block,此时直接索引要分配
	//cout << "add:" << addrr[2]<<" "<<addrr[1]<<" " << addrr[0] << endl;
	if (addrr[2] == -1 && addrr[1] == -1) {//不需要一级和二级索引，分配一个直接索引（0-5）
		int blkno = superblock.findFreeFile();//找到空闲block
		if (blkno == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[0]] = blkno;
	}
	else if (addrr[2] == -1 && addrr[1] != -1 && addrr[0] == 0) {//（6-7）一级索引要分配，直接索引要分配
		int blkno1 = superblock.findFreeFile();//找到空闲block
		int blkno2 = superblock.findFreeFile();//找到空闲block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[1]] = blkno1;
		int stack[INDEX_NUM];
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1内容读到stack（应是空）
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
	}
	else if (addrr[2] == -1 && addrr[1] != -1 && addrr[0] != 0) {//（6-7之间分配一个直接索引）直接索引分配
		int blkno = superblock.findFreeFile();//找到空闲block
		if (blkno == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack是一级索引内容
		stack[addrr[0]] = blkno;
		cachelist.writeCache(FILE_POS + inode.d_addr[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
	}
	else if (addrr[2] != -1 && addrr[1] == 0 && addrr[0] == 0) {//(8-9)三级索引都要分配
		int blkno1 = superblock.findFreeFile();//找到空闲block
		int blkno2 = superblock.findFreeFile();//找到空闲block
		int blkno3 = superblock.findFreeFile();//找到空闲block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		if (blkno3 == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[2]] = blkno1;
		unsigned int stack[INDEX_NUM];
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1内容读到stack（应是空）
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
		getblk(blkno2 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1内容读到stack（应是空）
		stack[0] = blkno3;
		cachelist.writeCache(blkno2 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
	}
	else if (addrr[2] != -1 && addrr[1] != 0 && addrr[0] == 0) {//(8-9)要分配一级索引和直接索引
		int blkno1 = superblock.findFreeFile();//找到空闲block
		int blkno2 = superblock.findFreeFile();//找到空闲block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack是二级索引内容
		stack[addrr[1]] = blkno1;
		cachelist.writeCache(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1内容读到stack（应是空）
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
	}
	else if (addrr[2] != -1 && addrr[0] != 0) {//直接索引分配
		//cout << 1 << endl;
		int blkno = superblock.findFreeFile();//找到空闲block
		//cout << blkno << endl;
		if (blkno == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack是二级索引内容
		int pos = FILE_POS + stack[addrr[1]];
		getblk(FILE_POS + stack[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack是一级索引内容
		stack[addrr[0]] = blkno;
		cachelist.writeCache(pos, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//内容写回cache
	}
}

//content写入fdnum
void fwrite(int fdnum,const char* content_, int size) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "文件句柄非法！" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "不存在文件句柄" << fdnum << endl;
		//throw(FILE_NOT_EXIST);
		return;
	}
	char* content = new char[size + 1];
	memcpy(content, content_, size);
	content[size] = '\0';
	
	DiskInode inode;//本file的inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	int size_con = size;
	//此时inode load完成
	if (inode.f_flag != inode.FWRITE) {
		cout << "文件非可写类型！" << endl;
		//throw(NAME_LONG);
		return;
	}

	int old = inode.f_offset;
	START:
	int offset = inode.f_offset;
	vector<int> addrr = off_to_block(offset);//offset定位到block
	//cout << addrr[2] << " " << addrr[1] << " " << addrr[0] << endl;
	int blkno;
	int stack[INDEX_NUM + 1];
	stack[INDEX_NUM ] = '\0';
	char readcontent[BLOCK_SIZE];
	if (addrr[2] != -1) {//存在二级索引
		blkno = inode.d_addr[addrr[2]];
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
	}
	if (addrr[1] != -1) {
		if (addrr[2] == -1) blkno = inode.d_addr[addrr[1]];
		else blkno = stack[addrr[1]];
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
	}
	if (addrr[0] != -1) {
		if(addrr[2] == -1 && addrr[1] == -1) blkno = inode.d_addr[addrr[0]];
		else blkno = stack[addrr[0]];
		
	}
	//此时readcontent加载完毕，offset更新完毕
	//int first = 0;
	//if (addrr[2] == -1 && addrr[1] == -1 && addrr[0] == 0) {//first page first 20 bits is filename
	//	first = 1;
	//}
	if (size_con + offset > BLOCK_SIZE) {
		if (offset != 0) {
			getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&readcontent, BLOCK_SIZE);
		}
		memcpy(readcontent + offset, content, BLOCK_SIZE - offset);
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, readcontent, BLOCK_SIZE);//内容写回cache
		inode.f_offset += (BLOCK_SIZE - offset);
		if (inode.f_offset >= inode.d_size) {//要新分配一个block
			new_block_to_file(inode);
			inode.d_size = inode.f_offset;
			//分配block,并且更新dsize
		}
		size_con -= (BLOCK_SIZE - offset);
		memcpy(content, content + (BLOCK_SIZE - offset), size_con);//将前面的content去掉
		//inode.d_size += size_con > BLOCK_SIZE ? BLOCK_SIZE : size_con;
		goto START;
	}
	else {
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&readcontent, BLOCK_SIZE);
		memcpy(readcontent + offset, content,size_con);
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, readcontent, offset + size_con);//内容写回cache
		//cout << "w_size:" << offset % (BLOCK_SIZE - first * FILENAME_LENGTH)<<" "<< size_con <<" "<< first * FILENAME_LENGTH << endl;
		inode.f_offset += size_con;
		inode.d_size = inode.d_size < inode.f_offset ? inode.f_offset : inode.d_size;
		inode.d_atime = time(NULL);//最后访问时间
	}


	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inode写入cache
	cout << "当前文件指针：" << old << "，写入字节数：" << size << endl;
	
}

string fread(int fdnum, int size) {
	string res="";
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "文件句柄非法！" << endl;
		//throw(NAME_LONG);
		return "";
	}
	if (open_file[fdnum] == -1) {
		cout << "不存在文件句柄" << fdnum << endl;
		//throw(FILE_NOT_EXIST);
		return "";
	}
	DiskInode inode;//本file的inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//此时inode load完成
	if (inode.f_offset >= inode.d_size) {//读完了
		inode.f_offset = 0;
	}
	if (size < 0) {
		size = inode.d_size - inode.f_offset;
	}
	size = (size > inode.d_size - inode.f_offset) ? inode.d_size - inode.f_offset : size;
	int rsize = size;
	
	int old = inode.f_offset;
START:
	int offset = inode.f_offset;
	vector<int> addrr = off_to_block(offset);//offset定位到block
	//cout << addrr[2] << " " << addrr[1] << " " << addrr[0] << endl;
	int blkno;
	unsigned int stack[INDEX_NUM + 1];
	stack[INDEX_NUM] = '\0';
	char readcontent[BLOCK_SIZE];
	if (addrr[2] != -1) {//存在二级索引
		blkno = inode.d_addr[addrr[2]];
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
	}
	if (addrr[1] != -1) {
		if (addrr[2] == -1) blkno = inode.d_addr[addrr[1]];
		else blkno = stack[addrr[1]];
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));
	}
	if (addrr[0] != -1) {
		if (addrr[2] == -1 && addrr[1] == -1) blkno = inode.d_addr[addrr[0]];
		else blkno = stack[addrr[0]];
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&readcontent, BLOCK_SIZE);
	}
	//此时readcontent加载完毕
	//for (int i = 0; i < BLOCK_SIZE; i++) {
	//	cout << (int)readcontent[i];
	//}
	//cout << endl;
	//int first = 0;
	//if (addrr[2] == -1 && addrr[1] == -1 && addrr[0] == 0) {//first page first 20 bits is filename
	//	first = 1;
	//}
	if (size + offset > BLOCK_SIZE) {
		//cout << 1 << endl;
		char* content = new char[BLOCK_SIZE - offset + 1];
		memcpy(content, readcontent + offset, BLOCK_SIZE - offset);
		content[BLOCK_SIZE - offset] = '\0';
		res += content;
		delete[]content;
		inode.f_offset += (BLOCK_SIZE - offset);
		size -= (BLOCK_SIZE - offset);
		//inode.d_size += size_con > BLOCK_SIZE ? BLOCK_SIZE : size_con;
		goto START;
	}
	else {
		//cout << 2 << endl;
		char* content = new char[size + 1];
		content[size] = '\0';
		memcpy(content, readcontent + offset, size);
		//cout << size << endl;
		//for (int i = 0; i < size; i++) {
		//	cout << (int)content[i];
		//}
		res += content;
		delete[]content;
		inode.f_offset += size;
		inode.d_atime = time(NULL);//最后访问时间
	}
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inode写入cache
	
	cout<< "当前文件指针：" << old << "，文件大小：" << inode.d_size << "，输出字节数：" << rsize << "\n输出：";
	//string tips = "当前文件指针：" + to_string(old) + "，文件大小：" + to_string(inode.d_size) + "，输出字节数：" + to_string(size) + "\n输出：";
	return res;
}

void exit() {
	cachelist.writeBack();//脏缓存写回
	//把三个全局变量写回，cachelist不需要写回，是属于内存的
	fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
	//如果没有打开文件则输出提示信息并throw错误
	if (!fd.is_open()) {
		cout << "无法打开文件卷" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}
	//写回superblock
	fd.seekg(SUPERBLOCK_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&superblock, sizeof(superblock));
	//写回directory
	int dic_inode_num = directory.d_inodenumber[0];//dictory的inode
	int dic_block_num;
	DiskInode dic_Inode;
	fd.seekg(INODE_POS * BLOCK_SIZE + dic_inode_num * INODE_SIZE, ios::beg);
	fd.read((char*)&dic_Inode, sizeof(dic_Inode));
	dic_block_num = dic_Inode.d_addr[0];
	fd.seekg((FILE_POS + dic_block_num) * BLOCK_SIZE, ios::beg);
	fd.write((char*)&directory, sizeof(directory));
}