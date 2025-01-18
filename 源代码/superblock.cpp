#include"head.h"

//�������ӷ��ҿյ�file�飬����file��ţ�0��ʼ��
//���Ѷ�Ӧ���ӿ����cache
int SuperBlock::findFreeFile() {
	if (s_nfree == 0) {//�����ӿ�
		int next_free = s_free[0];//��һ���ӿ�ĵ�ַ
		START:
		CacheBlock* pb = cachelist.findCache(next_free + FILE_POS,CacheBlock::BLOCK);//���Ƿ���cache��
		if (pb == NULL) {//cacheû�У���img
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//���û�д��ļ��������ʾ��Ϣ��throw����
			if (!fd.is_open()) {
				cout << "�޷����ļ���" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			int ps_free[FREE_INDEX_NUM + 20] = {0};//��img���ݶ������������
			int nfree;
			fd.seekg((FILE_POS + next_free) * BLOCK_SIZE, ios::beg);
			fd.read((char*)&ps_free, (FREE_INDEX_NUM+1) * sizeof(int));
			fd.close();
			nfree = ps_free[0];
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "û�ж���Ŀռ�����ļ���"<< endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//�ٿ���һ�����ӿ�
				next_free = ps_free[1];
				goto START;
			}
			else {//�����ӿ��пռ�
				int res = -1;
				for (int i = nfree + 1; i > 1; i--) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//��ʱ�Ѿ��õ�res��ֵ���ѱ���д��cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}

		}
		else {//cache������
			int ps_free[FREE_INDEX_NUM + 20] = {0};
			int nfree;
			cachelist.readCache(pb, (char*)ps_free);
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			//for (int i = 0; i < FREE_INDEX_NUM + 1; i++) {
			//	cout << ps_free[i] << " ";
			//	if (i % 8 == 7) cout << endl;
			//}
			//cout << endl;
			nfree = ps_free[0];
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "û�ж���Ŀռ�����ļ���" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//�ٿ���һ�����ӿ�
				next_free = ps_free[1];
				goto START;
			}
			else {//�����ӿ��пռ�
				int res = -1;
				for (int i = nfree + 1; i > 1; i--) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}				
				//��ʱ�Ѿ��õ�res��ֵ���ѱ���д��cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}
		}
	
	}
	else {
		//һ�������������ڴ棬�ͷŵ��ڴ�ӵ�����β������ֱ�ӷ���λ�ü���
		int res = s_free[s_nfree];
		s_free[s_nfree] = BLOCK_USED;//ֱ�Ӹ���superblock����
		s_nfree--;
		return res;
	}
}


//�������ӷ��ҿյ�inode�飬����inode��ţ�0��ʼ��
int SuperBlock::findFreeInode() {
	if (s_ninode == 0) {//�����ӿ�
		int next_free = s_inode[0];//��һ���ӿ�ĵ�ַ
		START:
		CacheBlock* pb = cachelist.findCache(next_free + FILE_POS, CacheBlock::BLOCK);//���Ƿ���cache��
		if (pb == NULL) {//cacheû�У���img
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//���û�д��ļ��������ʾ��Ϣ��throw����
			if (!fd.is_open()) {
				cout << "�޷����ļ���" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			int ps_free[FREE_INDEX_NUM + 2];//��img���ݶ������������
			int nfree;
			fd.seekg((FILE_POS + next_free) * BLOCK_SIZE, ios::beg);
			fd.read((char*)&ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
			fd.close();
			nfree = ps_free[0];
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "û�ж���Ŀռ�����ļ���" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//�ٿ���һ�����ӿ�
				next_free = ps_free[1];
				goto START;
			}
			else {//�����ӿ��пռ�
				int res;
				for (int i = nfree + 1; i > 1; i--) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//��ʱ�Ѿ��õ�res��ֵ���ѱ���д��cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}

		}
		else {//cache������
			int ps_free[FREE_INDEX_NUM + 2];
			cachelist.readCache(pb, (char*)ps_free);
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "û�ж���Ŀռ�����ļ���" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//�ٿ���һ�����ӿ�
				next_free = ps_free[1];
				goto START;
			}
			else {//�����ӿ��пռ�
				int res;
				for (int i = ps_free[0] + 1; i > 1; i--) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//��ʱ�Ѿ��õ�res��ֵ���ѱ���д��cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}
		}

	}
	else {
		//һ�������������ڴ棬�ͷŵ��ڴ�ӵ�����β������ֱ�ӷ���λ�ü���
		int res = s_inode[s_ninode];
		s_inode[s_ninode] = BLOCK_USED;//ֱ�Ӹ���superblock����
		s_ninode--;
		return res;
	}
}