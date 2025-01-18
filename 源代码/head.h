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
#define IMG_SIZE 8		//img��С128MB
#define BLOCK_SIZE 512		//һ������512�ֽ�
#define INODE_SIZE 64		//Innode��С64�ֽ�
#define SUPERBLOCK_POS 0	//superblock�����׵�ַ������	0h
#define INODE_POS 2			//Innode�����׵�ַ������		400h
#define FILE_POS 60			//File�����׵�ַ������			7800h
#define FREE_INDEX_NUM 100	//������������
#define BLOCK_USED -1
#define SUBDIRECTORY_NUM 20	//Ŀ¼�����ļ�������
#define FILENAME_LENGTH 20	//Ŀ¼���ļ����ֳ���
#define OPEN_FILE_NUM 50	//���ļ���������
#define INDEX_NUM 128		//��������128��512/4
#define SUCCESS 0
//Error
#define CANT_OPEN_FILE 1	//�򲻿��ļ�
#define NO_ROOM_FOR_FILE -2	//û�пռ�����ļ�
#define NAME_LONG 3			//����̫��
#define FILE_NOT_EXIST 4	//�ļ�������
#define OPEN_TOO_MANY_FILES 5//���ļ�����̫��
#define ILLEGAL_OFFSET 6	//���Ϸ���offset


class SuperBlock
{
	/* Functions */
public:
	///* Constructors */
	//SuperBlock();
	///* Destructors */
	//~SuperBlock();
	int findFreeFile();//�������ӷ��ҿյ�file�飬����file��ţ�0��ʼ��
	int findFreeInode();

	/* Members */
public:
	int		s_isize;		/* ���Inode��ռ�õ��̿��� */
	int		s_fsize;		/* file�����̿����� */

	int		s_nfree;		/* ֱ�ӹ���Ŀ����̿����� */
	int		s_free[FREE_INDEX_NUM];	/* ֱ�ӹ���Ŀ����̿������� ��0��ʼ���� */

	int		s_ninode;		/* ֱ�ӹ���Ŀ������Inode���� */
	int		s_inode[FREE_INDEX_NUM];	/* ֱ�ӹ���Ŀ������Inode������ */

	//int		s_flock;		/* ���������̿��������־ */
	//int		s_ilock;		/* ��������Inode���־ */

	//int		s_fmod;			/* �ڴ���super block�������޸ı�־����ζ����Ҫ��������Ӧ��Super Block */
	//int		s_ronly;		/* ���ļ�ϵͳֻ�ܶ��� */
	//int		s_time;			/* ���һ�θ���ʱ�� */
	int		padding[52 + (FREE_INDEX_NUM - 100)];	/* ���ʹSuperBlock���С����1024�ֽڣ�ռ��2������ */
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
		FILE = 0x1,//���ļ�
		DIRECTORY = 0x2//��Ŀ¼
	};
	enum FileFlags
	{
		FREAD = 0x1,			/* ���������� */
		FWRITE = 0x2,			/* д�������� */
		FPIPE = 0x4				/* �ܵ����� */
	};

	unsigned int d_mode;	/* ״̬�ı�־λ�������enum InodeMode */
	//int		d_nlink;		/* �ļ���������������ļ���Ŀ¼���в�ͬ·���������� */

	//���漰�û����֣�ȥ��
	//short	d_uid;			/* �ļ������ߵ��û���ʶ�� */
	//short	d_gid;			/* �ļ������ߵ����ʶ�� */

	unsigned int f_flag;		/* �Դ��ļ��Ķ���д����Ҫ�� */
	int		f_offset;			/* �ļ���дλ��ָ�� */

	int		d_size;			/* �ļ���С���ֽ�Ϊ��λ */
	int		d_addr[10];		/* �����ļ��߼���ú�������ת���Ļ��������� */

	time_t		d_atime;		/* ������ʱ�� */
	//time_tռ��8�ֽ�
};

class File
{
public:
	//char name[FILENAME_LENGTH];
	//int f_inodenumber;				//inode number of file(start from 0)
	char content[BLOCK_SIZE];//addr[0]��Ӧ��ǰFILENAME��char������
};

//Directory�ṹ��
class Directory 
{
public:
	char name[FILENAME_LENGTH];//Ŀ¼����
	int d_inodenumber[SUBDIRECTORY_NUM];		//��Ŀ¼Inode��	//-1
	char d_filename[SUBDIRECTORY_NUM][FILENAME_LENGTH];		//��Ŀ¼�ļ���	//\0
	//�ܹ���С500�ֽ�
};

//cache�����
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
	static const int BUFFER_SIZE = 512;     //��������С512�ֽ�
	enum CacheFlag
	{
		CB_DONE = 0x1,    //I/O��������
		CB_DELWRI = 0x2   //�ӳ�д
	};
	enum CacheType
	{
		INODE = 0x1,
		BLOCK = 0x2
	};
	unsigned int flags;   //������ƿ��־λ

	CacheBlock* forw;
	CacheBlock* back;

	int type;					//cahce��ŵ�ģ������
	int wcount;					//�贫�͵��ֽ���
	char buffer[BUFFER_SIZE];	//����������
	int	blkno;					//�����߼����
	int no;
};

class CacheList
{
public:
	static const int NBUF = 100;            //������ƿ� ������������
	static const int BUFFER_SIZE = 512;     //��������С512�ֽ�

	CacheList();
	~CacheList();
	void initList();		//init
	void cacheIn(CacheBlock* pb);	//���뻺���
	void cacheOut(CacheBlock* pb);	//�����out
	CacheBlock* findCache(int blkno, int type);//��blkno�Ƿ���ڻ��������
	void writeCache(int blkno, int type, const char* content, int length);
	//void writeBlock(int blkno);		//blkno��Ӧ��blockд�����
	int readCache(CacheBlock* pb, char* content);
	void freeCache(CacheBlock* pb);//�ͷŻ���飬�Żض�����
	void writeBack();//�໺��ȫ��д��img��

private:
	CacheBlock* bufferList;         //���ɻ�����п��ƿ飬���ҿջ������������
	CacheBlock nBuffer[NBUF];       //������ƿ����飬���еĻ���飬�����Ƿ���cache��ʱ����������

};

enum fseekMode {
	BEG = 0x1,//��ͷ
	CUR = 0x2,//��ǰ
	END = 0x3 //��β
};

//ȫ�ֱ���
extern Directory directory;//��ǰĿ¼
extern CacheList cachelist;//cache����
extern SuperBlock superblock;//��ǰ������
extern int open_file[OPEN_FILE_NUM];//���ļ���inode���

void init();
void mkdir(const char* name);	//�½��ļ���
void ls();				//��ǰĿ¼���ļ�
void fcreate(const char* name);//�½��ļ�
int fopen(const char* name);	//���ļ�
void fclose(int fdnum);	//�ر��ļ�
void exit();
void fseek(int fdnum, int dis, int type);//�ļ�ָ���ƶ�
vector<int> off_to_block(int& offset);//����offset�Ҷ�Ӧ��block
void getblk(const int blkno,int type, char* content, int length);//����blkno�õ����ݣ���cache���ң��Ҳ����Ͷ�img
void new_block_to_file(DiskInode& inode);//��file�ļ�����һ����block��ͬʱ����inode����cache
void fwrite(int fdnum, const char* content_, int size);
string fread(int fdnum, int size);
void cd(const char* name);
int fdelete(const char* name);
void fflag(int fdnum, string type);//�����ļ�Ȩ��
void newfile();