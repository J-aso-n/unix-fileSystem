#include"head.h"

CacheList::CacheList() {
	bufferList = new CacheBlock;
	initList();
}

CacheList::~CacheList() {
	
}

//ѭ������
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

//LRU�㷨��ÿ�δ�ͷ��ȡ����ʹ�ú�ŵ�β��
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

//��blkno�Ƿ���ڻ��������
CacheBlock* CacheList::findCache(int blkno,int type) {
	CacheBlock* pb = NULL;
	for (int i = 0; i < NBUF; ++i) {
		if (nBuffer[i].blkno == blkno && nBuffer[i].type == type) {//�Ѿ��ڻ��������
			pb = &nBuffer[i];
			break;
		}
	}
	return pb;
}

//��blkno�������contentд��cache
void CacheList::writeCache(int blkno,int type,const char* content,int length) {
	CacheBlock* pb = findCache(blkno, type);//���Ƿ���cache��
	if (pb == NULL) {//����ȡһ��cacheblockȥ��
		if (bufferList->back == bufferList) {
			//cout << "�޶����cache" << endl;
			//���໺��ȫ��д��img
			writeBack();
		}
		pb = bufferList->back;//ͷ
		cacheOut(pb);
		memcpy(pb->buffer, content, length);
		pb->type = type;
		pb->wcount = length;
		pb->blkno = blkno;
		pb->flags = pb->CB_DELWRI;//��������
		//memset(pb->buffer + length, 0, BUFFER_SIZE - length);//����ı�Ϊ0�����
	}
	else {//cache�����Ѿ��е�
		//int length = strlen(content) >= BUFFER_SIZE ? BUFFER_SIZE : strlen(content);
		memcpy(pb->buffer, content, length);
		pb->type = type;
		pb->wcount = length;
		pb->blkno = blkno;
		pb->flags = pb->CB_DELWRI;//��������
		memset(pb->buffer + length, 0, BUFFER_SIZE - length);//����ı�Ϊ0�����
	}
	//cout <<endl<< length << endl;
	//for (int j = 0; j < length; j++) {
	//	cout << (int)((char*)&pb->buffer)[j] << " ";
	//	if (j % 16 == 15)cout << endl;
	//}
}

//����pb������ݣ������к������ݣ�һ������,�����ֽ���
//���£������Ǻ��������ˣ���Ϊд��ʱ���Ե���blknoΪ׼������������
int CacheList::readCache(CacheBlock* pb, char* content) {
	memcpy(content, pb->buffer, pb->wcount);
	//cout << endl << "readcache" << endl;
	//for (int j = 0; j < pb->wcount; j++) {
	//	cout << (int)(content)[j] << " ";
	//	if (j % 16 == 15)cout << endl;
	//}
	return pb->wcount;
}

//�ͷŻ����
void CacheList::freeCache(CacheBlock* pb) {
	pb->reset();
	cacheIn(pb);
}

//�໺��ȫ��д��img��
void CacheList::writeBack() {
	fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
	//���û�д��ļ��������ʾ��Ϣ��throw����
	if (!fd.is_open()) {
		cout << "�޷����ļ���" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}

	for (int i = 0; i < NBUF; i++) {
		if (nBuffer[i].flags == nBuffer[i].CB_DELWRI) {//��
			if (nBuffer[i].type == nBuffer[i].INODE) {//inodeģ��
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