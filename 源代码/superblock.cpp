#include"head.h"

//成组链接法找空的file块，返回file块号（0开始）
//并把对应链接块放入cache
int SuperBlock::findFreeFile() {
	if (s_nfree == 0) {//找链接块
		int next_free = s_free[0];//下一链接块的地址
		START:
		CacheBlock* pb = cachelist.findCache(next_free + FILE_POS,CacheBlock::BLOCK);//看是否在cache中
		if (pb == NULL) {//cache没有，读img
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//如果没有打开文件则输出提示信息并throw错误
			if (!fd.is_open()) {
				cout << "无法打开文件卷" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			int ps_free[FREE_INDEX_NUM + 20] = {0};//将img内容读到这个变量中
			int nfree;
			fd.seekg((FILE_POS + next_free) * BLOCK_SIZE, ios::beg);
			fd.read((char*)&ps_free, (FREE_INDEX_NUM+1) * sizeof(int));
			fd.close();
			nfree = ps_free[0];
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "没有多余的空间分配文件！"<< endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//再看下一个链接块
				next_free = ps_free[1];
				goto START;
			}
			else {//本连接块有空间
				int res = -1;
				for (int i = nfree + 1; i > 1; i--) {//从后往前找第一个不是used的值
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//此时已经得到res的值，把本块写入cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}

		}
		else {//cache里面有
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
				cout << "没有多余的空间分配文件！" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//再看下一个链接块
				next_free = ps_free[1];
				goto START;
			}
			else {//本连接块有空间
				int res = -1;
				for (int i = nfree + 1; i > 1; i--) {//从后往前找第一个不是used的值
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}				
				//此时已经得到res的值，把本块写入cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}
		}
	
	}
	else {
		//一定是连续分配内存，释放的内存加到数组尾，所以直接返回位置即可
		int res = s_free[s_nfree];
		s_free[s_nfree] = BLOCK_USED;//直接更改superblock属性
		s_nfree--;
		return res;
	}
}


//成组链接法找空的inode块，返回inode块号（0开始）
int SuperBlock::findFreeInode() {
	if (s_ninode == 0) {//找链接块
		int next_free = s_inode[0];//下一链接块的地址
		START:
		CacheBlock* pb = cachelist.findCache(next_free + FILE_POS, CacheBlock::BLOCK);//看是否在cache中
		if (pb == NULL) {//cache没有，读img
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//如果没有打开文件则输出提示信息并throw错误
			if (!fd.is_open()) {
				cout << "无法打开文件卷" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			int ps_free[FREE_INDEX_NUM + 2];//将img内容读到这个变量中
			int nfree;
			fd.seekg((FILE_POS + next_free) * BLOCK_SIZE, ios::beg);
			fd.read((char*)&ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
			fd.close();
			nfree = ps_free[0];
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "没有多余的空间分配文件！" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//再看下一个链接块
				next_free = ps_free[1];
				goto START;
			}
			else {//本连接块有空间
				int res;
				for (int i = nfree + 1; i > 1; i--) {//从后往前找第一个不是used的值
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//此时已经得到res的值，把本块写入cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}

		}
		else {//cache里面有
			int ps_free[FREE_INDEX_NUM + 2];
			cachelist.readCache(pb, (char*)ps_free);
			ps_free[FREE_INDEX_NUM + 1] = '\0';
			if (ps_free[1] == 0 && ps_free[2] == BLOCK_USED) {
				cout << "没有多余的空间分配文件！" << endl;
				return NO_ROOM_FOR_FILE;
			}
			else if (ps_free[1] != 0 && ps_free[2] == BLOCK_USED) {//再看下一个链接块
				next_free = ps_free[1];
				goto START;
			}
			else {//本连接块有空间
				int res;
				for (int i = ps_free[0] + 1; i > 1; i--) {//从后往前找第一个不是used的值
					if (ps_free[i] != BLOCK_USED) {
						res = ps_free[i];
						ps_free[i] = BLOCK_USED;
						break;
					}
				}
				//此时已经得到res的值，把本块写入cache
				cachelist.writeCache(FILE_POS + next_free, CacheBlock::BLOCK, (char*)ps_free, (FREE_INDEX_NUM + 1) * sizeof(int));
				return res;
			}
		}

	}
	else {
		//一定是连续分配内存，释放的内存加到数组尾，所以直接返回位置即可
		int res = s_inode[s_ninode];
		s_inode[s_ninode] = BLOCK_USED;//直接更改superblock属性
		s_ninode--;
		return res;
	}
}