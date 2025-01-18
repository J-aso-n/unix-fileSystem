#include"head.h"

CacheList::CacheList() {
	bufferList = new CacheBlock;
	initList();
}

CacheList::~CacheList() {
	
}

//循环链表
void CacheList::initList()
{
	for (int i = 0; i < NBUF; i++) {
		if (i != 0)
			nBuffer[i].forw = nBuffer + i - 1;
		else {
			nBuffer[i].forw = bufferList;
			bufferList->back = nBuffer + i;
		}

		if (i + 1 < NBUF)
			nBuffer[i].back = nBuffer + i + 1;
		else {
			nBuffer[i].back = bufferList;
			bufferList->forw = nBuffer + i;
		}
		nBuffer[i].no = i;
	}
}

//LRU算法，每次从头部取出，使用后放到尾部
void CacheList::cacheOut(CacheBlock* pb)
{
	if (pb->back == NULL)
		return;
	pb->forw->back = pb->back;
	pb->back->forw = pb->forw;
	pb->back = NULL;
	pb->forw = NULL;
}

void CacheList::cacheIn(CacheBlock* pb)
{
	if (pb->back != NULL)
		return;
	pb->forw = bufferList->forw;
	pb->back = bufferList;
	bufferList->forw->back = pb;
	bufferList->forw = pb;
}

//看blkno是否存在缓存块中了
CacheBlock* CacheList::findCache(int blkno,int type) {
	CacheBlock* pb = NULL;
	for (int i = 0; i < NBUF; ++i) {
		if (nBuffer[i].blkno == blkno && nBuffer[i].type == type) {//已经在缓存块中了
			pb = &nBuffer[i];
			break;
		}
	}
	return pb;
}

//将blkno块的内容content写入cache
void CacheList::writeCache(int blkno,int type,const char* content,int length) {
	CacheBlock* pb = findCache(blkno, type);//看是否在cache中
	if (pb == NULL) {//新提取一个cacheblock去存
		if (bufferList->back == bufferList) {
			//cout << "无多余的cache" << endl;
			//把脏缓存全部写入img
			writeBack();
		}
		pb = bufferList->back;//头
		cacheOut(pb);
		memcpy(pb->buffer, content, length);
		pb->type = type;
		pb->wcount = length;
		pb->blkno = blkno;
		pb->flags = pb->CB_DELWRI;//打上脏标记
		//memset(pb->buffer + length, 0, BUFFER_SIZE - length);//后面的变为0，清空
	}
	else {//cache里面已经有的
		//int length = strlen(content) >= BUFFER_SIZE ? BUFFER_SIZE : strlen(content);
		memcpy(pb->buffer, content, length);
		pb->type = type;
		pb->wcount = length;
		pb->blkno = blkno;
		pb->flags = pb->CB_DELWRI;//打上脏标记
		memset(pb->buffer + length, 0, BUFFER_SIZE - length);//后面的变为0，清空
	}
	//cout <<endl<< length << endl;
	//for (int j = 0; j < length; j++) {
	//	cout << (int)((char*)&pb->buffer)[j] << " ";
	//	if (j % 16 == 15)cout << endl;
	//}
}

//读出pb里的内容，可能有后续内容，一并读出,返回字节数
//更新：不考虑后续内容了，因为写的时候以单个blkno为准，不考虑链接
int CacheList::readCache(CacheBlock* pb, char* content) {
	memcpy(content, pb->buffer, pb->wcount);
	//cout << endl << "readcache" << endl;
	//for (int j = 0; j < pb->wcount; j++) {
	//	cout << (int)(content)[j] << " ";
	//	if (j % 16 == 15)cout << endl;
	//}
	return pb->wcount;
}

//释放缓存块
void CacheList::freeCache(CacheBlock* pb) {
	pb->reset();
	cacheIn(pb);
}

//脏缓存全部写回img中
void CacheList::writeBack() {
	fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
	//如果没有打开文件则输出提示信息并throw错误
	if (!fd.is_open()) {
		cout << "无法打开文件卷" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}

	for (int i = 0; i < NBUF; i++) {
		if (nBuffer[i].flags == nBuffer[i].CB_DELWRI) {//脏
			if (nBuffer[i].type == nBuffer[i].INODE) {//inode模块
				fd.seekg(INODE_POS * BLOCK_SIZE + (nBuffer[i].blkno - INODE_POS) * INODE_SIZE, ios::beg);
				fd.write((char*)&(nBuffer[i].buffer), INODE_SIZE);
			}
			else {
				fd.seekg(nBuffer[i].blkno * BLOCK_SIZE, ios::beg);
				fd.write((char*)&(nBuffer[i].buffer), BLOCK_SIZE);
			}
			freeCache(&nBuffer[i]);
		}
	}

	fd.close();
}