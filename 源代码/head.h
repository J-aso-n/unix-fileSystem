#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include<iostream>
#include<time.h>
#include<fstream>
#include<iomanip>
#include<string>
#include<vector>
using namespace std;

/// <summary>
///  the whole img occupy 128MB
///  each Block size is 512 B
///  Superblock occupy 2 Block  0#-1#
///  8 Innode occupy 1 Block
///  plan to get 58*8 Innode    2#-59#
///  
/// </summary>

#define DISK_NAME "myDisk.img"
#define IMG_SIZE 8		//img大小128MB
#define BLOCK_SIZE 512		//一个扇区512字节
#define INODE_SIZE 64		//Innode大小64字节
#define SUPERBLOCK_POS 0	//superblock部分首地址扇区数	0h
#define INODE_POS 2			//Innode部分首地址扇区数		400h
#define FILE_POS 60			//File部分首地址扇区数			7800h
#define FREE_INDEX_NUM 100	//空闲索引数量
#define BLOCK_USED -1
#define SUBDIRECTORY_NUM 20	//目录的子文件最大个数
#define FILENAME_LENGTH 20	//目录子文件名字长度
#define OPEN_FILE_NUM 50	//打开文件的最大个数
#define INDEX_NUM 128		//索引个数128：512/4
#define SUCCESS 0
//Error
#define CANT_OPEN_FILE 1	//打不开文件
#define NO_ROOM_FOR_FILE -2	//没有空间分配文件
#define NAME_LONG 3			//名字太长
#define FILE_NOT_EXIST 4	//文件不存在
#define OPEN_TOO_MANY_FILES 5//打开文件个数太多
#define ILLEGAL_OFFSET 6	//不合法的offset


class SuperBlock
{
	/* Functions */
public:
	///* Constructors */
	//SuperBlock();
	///* Destructors */
	//~SuperBlock();
	int findFreeFile();//成组链接法找空的file块，返回file块号（0开始）
	int findFreeInode();

	/* Members */
public:
	int		s_isize;		/* 外存Inode区占用的盘块数 */
	int		s_fsize;		/* file部分盘块总数 */

	int		s_nfree;		/* 直接管理的空闲盘块数量 */
	int		s_free[FREE_INDEX_NUM];	/* 直接管理的空闲盘块索引表 从0开始计数 */

	int		s_ninode;		/* 直接管理的空闲外存Inode数量 */
	int		s_inode[FREE_INDEX_NUM];	/* 直接管理的空闲外存Inode索引表 */

	//int		s_flock;		/* 封锁空闲盘块索引表标志 */
	//int		s_ilock;		/* 封锁空闲Inode表标志 */

	//int		s_fmod;			/* 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block */
	//int		s_ronly;		/* 本文件系统只能读出 */
	//int		s_time;			/* 最近一次更新时间 */
	int		padding[52 + (FREE_INDEX_NUM - 100)];	/* 填充使SuperBlock块大小等于1024字节，占据2个扇区 */
};


class DiskInode
{
	/* Functions */
public:
	///* Constructors */
	//DiskInode();
	///* Destructors */
	//~DiskInode();

	/* Members */
public:
	enum InodeMode {
		FILE = 0x1,//是文件
		DIRECTORY = 0x2//是目录
	};
	enum FileFlags
	{
		FREAD = 0x1,			/* 读请求类型 */
		FWRITE = 0x2,			/* 写请求类型 */
		FPIPE = 0x4				/* 管道类型 */
	};

	unsigned int d_mode;	/* 状态的标志位，定义见enum InodeMode */
	//int		d_nlink;		/* 文件联结计数，即该文件在目录树中不同路径名的数量 */

	//不涉及用户部分，去掉
	//short	d_uid;			/* 文件所有者的用户标识数 */
	//short	d_gid;			/* 文件所有者的组标识数 */

	unsigned int f_flag;		/* 对打开文件的读、写操作要求 */
	int		f_offset;			/* 文件读写位置指针 */

	int		d_size;			/* 文件大小，字节为单位 */
	int		d_addr[10];		/* 用于文件逻辑块好和物理块好转换的基本索引表 */

	time_t		d_atime;		/* 最后访问时间 */
	//time_t占用8字节
};

class File
{
public:
	//char name[FILENAME_LENGTH];
	//int f_inodenumber;				//inode number of file(start from 0)
	char content[BLOCK_SIZE];//addr[0]对应的前FILENAME个char是名字
};

//Directory结构体
class Directory 
{
public:
	char name[FILENAME_LENGTH];//目录名字
	int d_inodenumber[SUBDIRECTORY_NUM];		//子目录Inode号	//-1
	char d_filename[SUBDIRECTORY_NUM][FILENAME_LENGTH];		//子目录文件名	//\0
	//总共大小500字节
};

//cache缓存块
class CacheBlock
{
public:
	CacheBlock() {
		flags = CB_DONE;
		forw = NULL;
		back = NULL;
		type = 0;
		wcount = 0;
		blkno = -1;
		no = 0;
		memset(buffer, 0, BUFFER_SIZE);
	}

	void reset() {
		flags = CB_DONE;
		forw = NULL;
		back = NULL;
		type = 0;
		wcount = 0;
		blkno = -1;
		memset(buffer, 0, sizeof buffer);
		//no = 0;
	}

public:
	static const int BUFFER_SIZE = 512;     //缓冲区大小512字节
	enum CacheFlag
	{
		CB_DONE = 0x1,    //I/O操作结束
		CB_DELWRI = 0x2   //延迟写
	};
	enum CacheType
	{
		INODE = 0x1,
		BLOCK = 0x2
	};
	unsigned int flags;   //缓存控制块标志位

	CacheBlock* forw;
	CacheBlock* back;

	int type;					//cahce存放的模块类型
	int wcount;					//需传送的字节数
	char buffer[BUFFER_SIZE];	//缓冲区数组
	int	blkno;					//磁盘逻辑块号
	int no;
};

class CacheList
{
public:
	static const int NBUF = 100;            //缓存控制块 缓冲区的数量
	static const int BUFFER_SIZE = 512;     //缓冲区大小512字节

	CacheList();
	~CacheList();
	void initList();		//init
	void cacheIn(CacheBlock* pb);	//插入缓存块
	void cacheOut(CacheBlock* pb);	//缓存块out
	CacheBlock* findCache(int blkno, int type);//看blkno是否存在缓存块中了
	void writeCache(int blkno, int type, const char* content, int length);
	//void writeBlock(int blkno);		//blkno对应的block写缓存块
	int readCache(CacheBlock* pb, char* content);
	void freeCache(CacheBlock* pb);//释放缓存块，放回队列中
	void writeBack();//脏缓存全部写回img中

private:
	CacheBlock* bufferList;         //自由缓存队列控制块，查找空缓存快在这里找
	CacheBlock nBuffer[NBUF];       //缓存控制块数组，所有的缓存块，查找是否在cache的时候在这里找

};

enum fseekMode {
	BEG = 0x1,//开头
	CUR = 0x2,//当前
	END = 0x3 //结尾
};

//全局变量
extern Directory directory;//当前目录
extern CacheList cachelist;//cache队列
extern SuperBlock superblock;//当前超级块
extern int open_file[OPEN_FILE_NUM];//打开文件的inode编号

void init();
void mkdir(const char* name);	//新建文件夹
void ls();				//当前目录下文件
void fcreate(const char* name);//新建文件
int fopen(const char* name);	//打开文件
void fclose(int fdnum);	//关闭文件
void exit();
void fseek(int fdnum, int dis, int type);//文件指针移动
vector<int> off_to_block(int& offset);//根据offset找对应的block
void getblk(const int blkno,int type, char* content, int length);//根据blkno得到内容，在cache中找，找不到就读img
void new_block_to_file(DiskInode& inode);//给file文件分配一个新block，同时更新inode，进cache
void fwrite(int fdnum, const char* content_, int size);
string fread(int fdnum, int size);
void cd(const char* name);
int fdelete(const char* name);
void fflag(int fdnum, string type);//更改文件权限
void newfile();