#include"head.h"

void init() {
	fstream fd(DISK_NAME, ios::out);//���
	fd.close();
	fd.open(DISK_NAME, ios::out | ios::in | ios::binary);

	//���û�д��ļ��������ʾ��Ϣ��throw����
	if (!fd.is_open()) {
		cout << "�޷����ļ���" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}
	//cout << "success!" << endl;

	//SuperBlock init
	//SuperBlock superblock;
	superblock.s_isize = (FILE_POS - 1 - INODE_POS + 1);	/* ���Inode��ռ�õ��̿��� 2# - 59# mention innode number should * 8 */
	superblock.s_fsize = IMG_SIZE * 2 * 1024 - 1 - FILE_POS + 1;	/* file�����̿����� 60# - 128*2*1024-1 # */
	//superblock.s_fsize = IMG_SIZE * 2 - 1 - FILE_POS + 1;	/* ���� */
	superblock.s_nfree = FREE_INDEX_NUM;				/* ֱ�ӹ���Ŀ����̿����� */
	for (int i = 0; i < FREE_INDEX_NUM; i++) {
		superblock.s_free[i] = FREE_INDEX_NUM - i - 1;
	}
	//��ʱs_free[0] = 99,��������Ҫ��s_free[0]ָ����һ�����ӵĵ�ַ������Ҫ�ĵ�
	superblock.s_free[0] = 1; //FILE���ֵڶ���block����װs-free����һ������
	//s_nfree�����Ӻ����

	superblock.s_ninode = FREE_INDEX_NUM - 1;				/* ֱ�ӹ���Ŀ������Inode���� */
	for (int i = 0; i < FREE_INDEX_NUM; i++) {
		superblock.s_inode[i] = FREE_INDEX_NUM - i - 1;
	}
	//��ʼ��������
	int used_block = 0;
	int block_pos = FREE_INDEX_NUM - 1;
	//����file����
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
		//д���ڴ�
		if (i != i_max)
			fd.seekg((FILE_POS + stack[0] - 1) * BLOCK_SIZE, ios::beg);
		else
			fd.seekg((FILE_POS + i_max - 1) * BLOCK_SIZE, ios::beg);
		fd.write((char*)&nfree, sizeof(int));
		fd.write((char*)&stack, FREE_INDEX_NUM * sizeof(int));
	}
	//����innode����
	block_pos = FREE_INDEX_NUM - 1;
	superblock.s_inode[0] = i_max ;//innode��һ����λ��
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
		//д���ڴ�
		if (i != i_max2)
			fd.seekg((FILE_POS + stack[0] - 1) * BLOCK_SIZE, ios::beg);
		else
			fd.seekg((FILE_POS + superblock.s_inode[0] + i_max2 - 2) * BLOCK_SIZE, ios::beg);
		fd.write((char*)&nfree, sizeof(int));
		fd.write((char*)&stack, FREE_INDEX_NUM * sizeof(int));
	}
	used_block++;//���˵�block��
	if (used_block > 99) {//Ҫɾ�����ӿ�Ŀ��п�
		used_block -= 99;
		superblock.s_nfree = 0;
		memset(superblock.s_free + 1, BLOCK_USED, (FREE_INDEX_NUM - 1) * sizeof(int));
		//����ɾ���ӿ�
		int next_free = superblock.s_free[0];//��һ���ӿ�ĵ�ַ
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
		//ɾ�����д��cache
		cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
		if (used_block > 0) {//����ɾ����һ�����ӿ�
			next_free = stack[1];
			goto START;
		}
	}
	else {
		superblock.s_nfree -= used_block;
		//����useblock+1��
		for (int i = 0; i < used_block; i++) {
			superblock.s_free[FREE_INDEX_NUM - i - 1] = BLOCK_USED;
		}
	}
	//����superblock��ʼ����ϣ����ǻ�ûд���ڴ�

	//������Ŀ¼inode
	DiskInode Inode_root;
	Inode_root.d_mode = DiskInode::DIRECTORY;//Ŀ¼
	//Inode_root.d_nlink = 0;//�������
	//Inode_root.d_uid = 0;//����Ա
	//Inode_root.d_gid = 1;//�ļ������ߵ����ʶ
	Inode_root.f_flag = 0;//�����ļ�
	Inode_root.f_offset = 0;
	Inode_root.d_size = 0;//Ŀ¼��СΪ0
	Inode_root.d_addr[0] = 0;//��Ŀ¼��ӦBlock
	Inode_root.d_atime = time(NULL);//������ʱ��
	//Inode_root.d_mtime = time(NULL);//������ʱ��
	//д���ڴ�
	fd.seekg(INODE_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&Inode_root, sizeof(Inode_root));
	//��Ŀ¼blockд��0��block
	Directory root_directory;
	strcpy(root_directory.name, "root");
	strcpy(root_directory.d_filename[0], ".");//0���Լ�
	root_directory.d_inodenumber[0] = 0;
	strcpy(root_directory.d_filename[1], "..");//1�Ǹ��ף��˴������Լ���
	root_directory.d_inodenumber[1] = 0;
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		root_directory.d_filename[i][0] = '\0';
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		root_directory.d_inodenumber[i] = -1;
	}
	fd.seekg(FILE_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&root_directory, sizeof(root_directory));
	//����file���֣�inode���ֳ�ʼ�����
	//0��innode����
	//superblock.s_nfree -= 1;
	superblock.s_ninode -= 1;
	superblock.s_inode[FREE_INDEX_NUM - 1] = BLOCK_USED;
	fd.seekg(SUPERBLOCK_POS* BLOCK_SIZE, ios::beg);
	fd.write((char*)&superblock, sizeof(superblock));


	//�����Ŀ¼
	fd.seekg(FILE_POS* BLOCK_SIZE, ios::beg);
	fd.read((char*)&directory, sizeof(directory));
	fd.close();

	//��ʼ��openfile
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

//����blkno�õ����ݣ���cache���ң��Ҳ����Ͷ�img,��д��cache��
void getblk(const int blkno, int type, char* content, int length) {
	//cout << "getblk:" << blkno << " " << type << endl;
	CacheBlock* pb = cachelist.findCache(blkno, type);
	//cout << "getblk:pb:" << " " << pb->blkno << " " << pb->type << endl;
	if (pb != NULL) {//cache���������block
		cachelist.readCache(pb, content);
	}
	else {
		fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
		//���û�д��ļ��������ʾ��Ϣ��throw����
		if (!fd.is_open()) {
			cout << "�޷����ļ���" << DISK_NAME << endl;
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

//������Ŀ¼
void mkdir(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "Ŀ¼���ƷǷ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	//�鿴��ǰĿ¼�������ͬ������
	bool cant_create = true;//Ŀ¼�ļ���Ŀ�ﵽ����
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {
			cout << "ͬ���ļ����Ѵ��ڣ�" << endl;
			return;
		}
		if (directory.d_filename[i][0] == '\0') {
			cant_create = false;
		}
	}
	if (cant_create == true) {
		cout << "Ŀ¼�ļ���Ŀ�Ѵ����ޣ�" << endl;
		return;
	}
	int freeBlock = superblock.findFreeFile();//����block���
	int freeInode = superblock.findFreeInode();//����indode���
	if (freeBlock == NO_ROOM_FOR_FILE) return;
	if (freeInode == NO_ROOM_FOR_FILE) return;
	//����Ŀ¼inode
	DiskInode new_Inode;
	new_Inode.d_mode = DiskInode::DIRECTORY;//Ŀ¼
	new_Inode.f_flag = 0;
	new_Inode.f_offset = 0;
	new_Inode.d_size = 0;//Ŀ¼��СΪ0
	new_Inode.d_addr[0] = freeBlock;//Ŀ¼��ӦBlock
	new_Inode.d_atime = time(NULL);//������ʱ��
	//����Ŀ¼
	Directory new_directory;
	strcpy(new_directory.name, name);
	strcpy(new_directory.d_filename[0], ".");//0���Լ�
	new_directory.d_inodenumber[0] = freeInode;
	strcpy(new_directory.d_filename[1], "..");//1�Ǹ���
	new_directory.d_inodenumber[1] = directory.d_inodenumber[0];//��ǰĿ¼��inode���Ǹ���
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		new_directory.d_filename[i][0] = '\0';
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		new_directory.d_inodenumber[i] = -1;
	}
	//�޸ĵ�ǰĿ¼
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] == '\0') {
			strcpy(directory.d_filename[i], new_directory.name);
			directory.d_inodenumber[i] = freeInode;
			//�������Ҫ����һ�����ֳ�������
			break;
		}
	}
	//directory��ȫ�ֱ�������д��cache�У�exit��ʱ��ֱ��д��img����

	//new_dictoryд��cache
	cachelist.writeCache(freeBlock + FILE_POS, CacheBlock::BLOCK, (char*)&new_directory, sizeof new_directory);
	//inodeд��cache
	cachelist.writeCache(freeInode + INODE_POS, CacheBlock::INODE, (char*)&new_Inode, sizeof new_Inode);
	//���
	cout << "�½��ļ��� "<<name<<" ���" << endl;
}

//��Ŀ¼
void ls() {
	cout << "��ǰĿ¼: " << directory.name << endl;
	cout << "��Ŀ¼��" << endl;
	cout << "**********************************************" << endl;
	cout << "  �ļ�����    �ļ���    ������ʱ��" << endl;
	cout << "**********************************************" << endl;
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] != '\0') {
			//cout <<directory.d_inodenumber[i] << " " << directory.d_filename[i] << endl;
			DiskInode inode;//��file��inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
			if (inode.d_mode == DiskInode::DIRECTORY) {
				cout << "  �ļ���      ";
			}
			else if (inode.d_mode == DiskInode::FILE) {
				cout << "  �ļ�        ";
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
	if (strcmp(name, "..") == 0) {//�ص��ϲ��ļ���
		DiskInode parent;//parent��inode
		getblk(directory.d_inodenumber[1] + INODE_POS, CacheBlock::INODE, (char*)&parent, sizeof parent);
		DiskInode old;//���ڵ�inode
		getblk(directory.d_inodenumber[0] + INODE_POS, CacheBlock::INODE, (char*)&old, sizeof old);
		cachelist.writeCache(old.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//��directoryд��cache
		getblk(parent.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//directory����		
		//for (int j = 0; j < sizeof directory; j++) {
		//	cout << (int)((char*)&directory)[j] << " ";
		//	if (j % 16 == 15)cout << endl;
		//}
		return;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {//�ҵ����ļ���
			DiskInode new_inode;//��file��inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&new_inode, sizeof new_inode);
			if (new_inode.d_mode != DiskInode::DIRECTORY) {
				continue;
			}
			DiskInode old;//��directory��inode
			getblk(directory.d_inodenumber[0] + INODE_POS, CacheBlock::INODE, (char*)&old, sizeof old);
			cachelist.writeCache(old.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//��directoryд��cache

			getblk(new_inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//directory����
			
			//cout << endl << "directory" << endl;
			//for (int j = 0; j < sizeof directory; j++) {
			//	cout << (int)((char*)&directory)[j] << " ";
			//	if (j % 16 == 15)cout << endl;
			//}
			
			return;
		}
	}
	cout << "û���ļ���" << name << endl;
	return;
}

//�½��ļ�
void fcreate(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "�ļ����ƷǷ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	//�鿴��ǰĿ¼�������ͬ������
	bool cant_create = true;//Ŀ¼�ļ���Ŀ�ﵽ����
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(directory.d_filename[i], name) == 0) {
			cout << "ͬ���ļ��Ѵ��ڣ�" << endl;
			return;
		}
		if (directory.d_filename[i][0] == '\0') {
			cant_create = false;
		}
	}
	if (cant_create == true) {
		cout << "Ŀ¼�ļ���Ŀ�Ѵ����ޣ�" << endl;
		return;
	}
	int freeBlock = superblock.findFreeFile();//����block���
	int freeInode = superblock.findFreeInode();//����indode���
	if (freeBlock == NO_ROOM_FOR_FILE) return;
	if (freeInode == NO_ROOM_FOR_FILE) return;
	//����Ŀ¼inode
	DiskInode new_Inode;
	new_Inode.d_mode = DiskInode::FILE;//Ŀ¼
	//new_Inode.d_nlink = 0;//�������
	//new_Inode.d_uid = 0;//����Ա
	//new_Inode.d_gid = 1;//�ļ������ߵ����ʶ
	new_Inode.f_flag = DiskInode::FWRITE;//�ɶ�д
	new_Inode.f_offset = 0;
	new_Inode.d_size = 0;//��ǰ�ļ���СΪ0
	new_Inode.d_addr[0] = freeBlock;//�ļ���ӦBlock
	new_Inode.d_atime = time(NULL);//������ʱ��
	//�����ļ�
	File new_File;
	//memcpy(new_File.content, name, sizeof name);
	//strcpy(new_File.content, name);

	//�޸ĵ�ǰĿ¼
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (directory.d_filename[i][0] == '\0') {
			strcpy(directory.d_filename[i], name);//0���Լ�
			//memcpy(directory.d_filename[i], name, strlen(name));
			//strcpy(directory.d_filename[i], new_File.name);
			directory.d_inodenumber[i] = freeInode;
			//�������Ҫ����һ�����ֳ�������
			break;
		}
	}
	//new_Fileд��cache
	cachelist.writeCache(freeBlock + FILE_POS, CacheBlock::BLOCK, (char*)&new_File, sizeof new_File);
	//inodeд��cache
	cachelist.writeCache(freeInode + INODE_POS, CacheBlock::INODE, (char*)&new_Inode, sizeof new_Inode);
	//���
	cout << "�½��ļ� "<<name<<" ���" << endl;
}

int fopen(const char* name) {
	if (strlen(name) > FILENAME_LENGTH || strlen(name) <= 0) {
		cout << "�ļ����ƷǷ���" << endl;
		//throw(NAME_LONG);
		return -1;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(name, directory.d_filename[i]) == 0) {//Ŀ¼��������ļ�
			int pos = 0;
			for (; pos < OPEN_FILE_NUM; pos++) {
				if (open_file[pos] == -1) {//�յ�pair
					break;
				}
			}
			if (pos == OPEN_FILE_NUM) {
				cout << "���ļ������Ѵ�����" << endl;
				return OPEN_TOO_MANY_FILES;
			}

			open_file[pos] = directory.d_inodenumber[i];
			cout << "���ļ� " << name << " �ɹ����ļ���� " << pos << endl;
			return pos;
		}
	}
	//Ŀ¼��û������ļ�
	cout << "�������ļ� " << name << endl;
	return -1;
}

//�ر�fdnum��Ӧ��fstream
void fclose(int fdnum) {
	if(fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "�ļ�����Ƿ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "�������ļ����" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	open_file[fdnum] = -1;
	cachelist.writeBack();//ˢһ�λ���
	cout << "�ر��ļ���� " << fdnum << " �ɹ�" << endl;
}

//�����ļ���дȨ��
void fflag(int fdnum, string type) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "�ļ�����Ƿ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "�������ļ����" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	DiskInode inode;//��file��inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//��ʱinode load���
	if (type == "read") {
		inode.f_flag = inode.FREAD;
		cout << "�ļ�Ȩ���Ѹ���Ϊֻ����" << endl;
	}
	else if (type == "write") {
		inode.f_flag = inode.FWRITE;
		cout << "�ļ�Ȩ���Ѹ���Ϊ��д��" << endl;
	}
	else {
		cout << "Ȩ��������ȷ��" << endl;
		return;
	}
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inodeд��cache
	
}

//�ļ�ָ���ƶ�dis����
void fseek(int fdnum, int dis, int type) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "�ļ�����Ƿ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "�������ļ����" << fdnum << endl;
		throw(FILE_NOT_EXIST);
	}
	DiskInode inode;//��file��inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//��ʱinode load���
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
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inodeд��cache
	cout << "�ļ�ָ���ƶ��� " << inode.f_offset << " ,ԭ���ļ�ָ��λ�� " << old << " ,�ƶ����� " << dis << endl;
}

//inode���ļ�inode��int i����inode��Ӧ��directory��λ�ã�����ɾ��inode��
inline void delete_file(const DiskInode inode, int i) {
	char blank[BLOCK_SIZE] = { '\0' };
	vector<int> reset_blk;
	int stack[INDEX_NUM + 2];
	stack[INDEX_NUM + 1] = '\0';
	int nfree;
	for (int j = 0; j <= inode.d_size; j += BLOCK_SIZE) {
		int offset = j;
		vector<int> addrr = off_to_block(offset);//offset��λ��block
		int blkno;
		
		if (addrr[2] != -1) {//���ڶ�������
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
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//��ԭ����ҳ��ɿհ�ҳ
		//���潫���е�block�����superblock
		if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//Ҫ����һ�����ӿ�
			int next_free = superblock.s_free[0];//��һ���ӿ�ĵ�ַ
		START:
			getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
			nfree = stack[0];
			if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
				next_free = stack[1];
				goto START;
			}
			else {
				for (int j = 2; j <= nfree + 1; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
					if (stack[j] == BLOCK_USED) {
						stack[j] = blkno;
						break;
					}
				}
				//������д��cache
				cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
			}
		}
		else {
			superblock.s_free[++superblock.s_nfree] = blkno;
		}
	}

	//�����inode
	cachelist.writeCache(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, blank, sizeof inode);//inodeд��cache
	//��inode��Ӧ��ҳ�ŷŻ�superblock
	if (superblock.s_ninode == FREE_INDEX_NUM) {//Ҫ����һ�����ӿ�
		int next_free = superblock.s_inode[0];//��һ���ӿ�ĵ�ַ
		int stack[FREE_INDEX_NUM+2];
		stack[FREE_INDEX_NUM + 1] = '\0';
		int nfree;
		START1:
		getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
		nfree = stack[0];
		if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
			next_free = stack[1];
			goto START1;
		}
		else {
			for (int j = 2; j <= nfree + 1; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
				if (stack[j] == BLOCK_USED) {
					stack[j] = directory.d_inodenumber[i];
					break;
				}
			}
			//������д��cache
			cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
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
		if (directory.d_inodenumber[j] != -1) {//�ж�����
			DiskInode mid;//��file��inode
			getblk(directory.d_inodenumber[j] + INODE_POS, CacheBlock::INODE, (char*)&mid, sizeof mid);
			if (mid.d_mode == mid.FILE) {//���ļ�
				delete_file(mid, j);
			}
			else {//���ļ���
				Directory old;
				memcpy(&old, &directory, sizeof directory);//����һ������
				getblk(mid.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//�õ���Ŀ¼
				delete_directory();
				memcpy(&directory, &old, sizeof directory);//�ص�

				//Ŀ¼ҳ��Ϊ�հ�ҳ
				cachelist.writeCache(mid.d_addr[0] + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//��ԭ����ҳ��ɿհ�ҳ
				//���潫���е�block�����superblock
				if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//Ҫ����һ�����ӿ�
					int next_free = superblock.s_free[0];//��һ���ӿ�ĵ�ַ
				START:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
						next_free = stack[1];
						goto START;
					}
					else {
						for (int j = 2; j <= nfree + 1 ; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
							if (stack[j] == BLOCK_USED) {
								stack[j] = mid.d_addr[0];
								break;
							}
						}
						//������д��cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
					}
				}
				else
					superblock.s_free[++superblock.s_nfree] = mid.d_addr[0];

				//����ļ��е�inode
				cachelist.writeCache(directory.d_inodenumber[j] + INODE_POS, CacheBlock::INODE, blank, sizeof mid);//inodeд��cache
				//��inode��Ӧ��ҳ�ŷŻ�superblock
				if (superblock.s_ninode == FREE_INDEX_NUM - 1) {//Ҫ����һ�����ӿ�
					int next_free = superblock.s_inode[0];//��һ���ӿ�ĵ�ַ
					int stack[FREE_INDEX_NUM+2];
					stack[FREE_INDEX_NUM + 1] = '\0';
					int nfree;
				START1:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM+1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
						next_free = stack[1];
						goto START1;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
							if (stack[j] == BLOCK_USED) {
								stack[j] = directory.d_inodenumber[j];
								break;
							}
						}
						//������д��cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
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
		cout << "�ļ����ƷǷ���" << endl;
		//throw(NAME_LONG);
		return NAME_LONG;
	}
	for (int i = 2; i < SUBDIRECTORY_NUM; i++) {
		if (strcmp(name, directory.d_filename[i]) == 0) {//Ŀ¼��������ļ�
			for (int pos = 0; pos < OPEN_FILE_NUM; pos++) {
				if (open_file[pos] == directory.d_inodenumber[i]) {//���ļ��Ѿ�����
					fclose(pos);
					break;
				}
			}
			//����cache��directory
			DiskInode inode;//��file��inode
			getblk(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
			//ȡ�ñ��ļ�inode
			if (inode.d_mode == inode.FILE) {//���ļ�
				delete_file(inode, i);
			}
			else {//���ļ���
				Directory old;
				memcpy(&old, &directory, sizeof directory);//����һ������
				getblk(inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, (char*)&directory, sizeof directory);//�õ���Ŀ¼
				delete_directory();
				memcpy(&directory, &old, sizeof directory);//�ص�

				char blank[BLOCK_SIZE] = { '\0' };
				int nfree;
				int stack[FREE_INDEX_NUM + 2];
				stack[FREE_INDEX_NUM + 1] = '\0';
				//Ŀ¼ҳ��Ϊ�հ�ҳ
				cachelist.writeCache(inode.d_addr[0] + FILE_POS, CacheBlock::BLOCK, blank, BLOCK_SIZE);//��ԭ����ҳ��ɿհ�ҳ
				//���潫���е�block�����superblock
				if (superblock.s_nfree == FREE_INDEX_NUM - 1) {//Ҫ����һ�����ӿ�
					int next_free = superblock.s_free[0];//��һ���ӿ�ĵ�ַ
				START:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
						next_free = stack[1];
						goto START;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
							if (stack[j] == BLOCK_USED) {
								stack[j] = inode.d_addr[0];
								break;
							}
						}
						//������д��cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM + 1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
					}
				}
				else
					superblock.s_free[++superblock.s_nfree] = inode.d_addr[0];

				//����ļ��е�inode
				cachelist.writeCache(directory.d_inodenumber[i] + INODE_POS, CacheBlock::INODE, blank, sizeof inode);//inodeд��cache
				//��inode��Ӧ��ҳ�ŷŻ�superblock
				if (superblock.s_ninode == FREE_INDEX_NUM - 1) {//Ҫ����һ�����ӿ�
					int next_free = superblock.s_inode[0];//��һ���ӿ�ĵ�ַ
					int nfree;
					int stack[FREE_INDEX_NUM + 2];
					stack[FREE_INDEX_NUM + 1] = '\0';
				START1:
					getblk(next_free + FILE_POS, CacheBlock::BLOCK, (char*)&stack, (FREE_INDEX_NUM + 1) * sizeof(int));
					nfree = stack[0];
					if (stack[nfree + 1] != BLOCK_USED) {//������һ�����ӿ�
						next_free = stack[1];
						goto START1;
					}
					else {
						for (int j = 2; j <= nfree + 1; j++) {//�Ӻ���ǰ�ҵ�һ������used��ֵ
							if (stack[j] == BLOCK_USED) {
								stack[j] = directory.d_inodenumber[j];
								break;
							}
						}
						//������д��cache
						cachelist.writeCache(next_free + FILE_POS, CacheBlock::BLOCK, (char*)stack, (FREE_INDEX_NUM+1) * sizeof(int));//��ԭ����ҳ��ɿհ�ҳ
					}
				}
				else
					superblock.s_inode[++superblock.s_ninode] = directory.d_inodenumber[i];
			}

			
			//����Ŀ¼
			directory.d_inodenumber[i] = -1;
			directory.d_filename[i][0] = '\0';

			cout << "ɾ���ļ�(��) " << name << " �ɹ���" << endl;
			return SUCCESS;
		}
	}
	//no this file
	cout << "�������ļ� " << name << endl;
	return FILE_NOT_EXIST;
}

//����offset�Ҷ�Ӧ��block,[0]��Ӧֱ��������[1]��Ӧһ������,[2]��Ӧ2������(ע�����±�)
vector<int> off_to_block(int& offset) {
	vector<int> res(3,0);
	if (offset >= 0 && offset < 6 * BLOCK_SIZE) {
		res[1] = -1; res[2] = -1;//ֻ��Ҫֱ������
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
		res[2] = -1;//Ҫһ��������ֱ������
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
	else {//offset������߹�С
		cout << "���Ϸ���offset" << endl;
		throw(ILLEGAL_OFFSET);
	}
	return res;
}

//��file�ļ�����һ����block��ͬʱ����inode����cache
void new_block_to_file(DiskInode& inode) {
	int offset = inode.f_offset;
	vector<int> addrr = off_to_block(offset);//offset��λ��block,��ʱֱ������Ҫ����
	//cout << "add:" << addrr[2]<<" "<<addrr[1]<<" " << addrr[0] << endl;
	if (addrr[2] == -1 && addrr[1] == -1) {//����Ҫһ���Ͷ�������������һ��ֱ��������0-5��
		int blkno = superblock.findFreeFile();//�ҵ�����block
		if (blkno == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[0]] = blkno;
	}
	else if (addrr[2] == -1 && addrr[1] != -1 && addrr[0] == 0) {//��6-7��һ������Ҫ���䣬ֱ������Ҫ����
		int blkno1 = superblock.findFreeFile();//�ҵ�����block
		int blkno2 = superblock.findFreeFile();//�ҵ�����block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[1]] = blkno1;
		int stack[INDEX_NUM];
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1���ݶ���stack��Ӧ�ǿգ�
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
	}
	else if (addrr[2] == -1 && addrr[1] != -1 && addrr[0] != 0) {//��6-7֮�����һ��ֱ��������ֱ����������
		int blkno = superblock.findFreeFile();//�ҵ�����block
		if (blkno == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack��һ����������
		stack[addrr[0]] = blkno;
		cachelist.writeCache(FILE_POS + inode.d_addr[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
	}
	else if (addrr[2] != -1 && addrr[1] == 0 && addrr[0] == 0) {//(8-9)����������Ҫ����
		int blkno1 = superblock.findFreeFile();//�ҵ�����block
		int blkno2 = superblock.findFreeFile();//�ҵ�����block
		int blkno3 = superblock.findFreeFile();//�ҵ�����block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		if (blkno3 == NO_ROOM_FOR_FILE) return;
		inode.d_addr[addrr[2]] = blkno1;
		unsigned int stack[INDEX_NUM];
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1���ݶ���stack��Ӧ�ǿգ�
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
		getblk(blkno2 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1���ݶ���stack��Ӧ�ǿգ�
		stack[0] = blkno3;
		cachelist.writeCache(blkno2 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
	}
	else if (addrr[2] != -1 && addrr[1] != 0 && addrr[0] == 0) {//(8-9)Ҫ����һ��������ֱ������
		int blkno1 = superblock.findFreeFile();//�ҵ�����block
		int blkno2 = superblock.findFreeFile();//�ҵ�����block
		if (blkno1 == NO_ROOM_FOR_FILE) return;
		if (blkno2 == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack�Ƕ�����������
		stack[addrr[1]] = blkno1;
		cachelist.writeCache(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
		getblk(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//blkno1���ݶ���stack��Ӧ�ǿգ�
		stack[0] = blkno2;
		cachelist.writeCache(blkno1 + FILE_POS, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
	}
	else if (addrr[2] != -1 && addrr[0] != 0) {//ֱ����������
		//cout << 1 << endl;
		int blkno = superblock.findFreeFile();//�ҵ�����block
		//cout << blkno << endl;
		if (blkno == NO_ROOM_FOR_FILE) return;
		unsigned int stack[INDEX_NUM];
		getblk(FILE_POS + inode.d_addr[addrr[2]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack�Ƕ�����������
		int pos = FILE_POS + stack[addrr[1]];
		getblk(FILE_POS + stack[addrr[1]], CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//stack��һ����������
		stack[addrr[0]] = blkno;
		cachelist.writeCache(pos, CacheBlock::BLOCK, (char*)&stack, INDEX_NUM * sizeof(int));//����д��cache
	}
}

//contentд��fdnum
void fwrite(int fdnum,const char* content_, int size) {
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "�ļ�����Ƿ���" << endl;
		//throw(NAME_LONG);
		return;
	}
	if (open_file[fdnum] == -1) {
		cout << "�������ļ����" << fdnum << endl;
		//throw(FILE_NOT_EXIST);
		return;
	}
	char* content = new char[size + 1];
	memcpy(content, content_, size);
	content[size] = '\0';
	
	DiskInode inode;//��file��inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	int size_con = size;
	//��ʱinode load���
	if (inode.f_flag != inode.FWRITE) {
		cout << "�ļ��ǿ�д���ͣ�" << endl;
		//throw(NAME_LONG);
		return;
	}

	int old = inode.f_offset;
	START:
	int offset = inode.f_offset;
	vector<int> addrr = off_to_block(offset);//offset��λ��block
	//cout << addrr[2] << " " << addrr[1] << " " << addrr[0] << endl;
	int blkno;
	int stack[INDEX_NUM + 1];
	stack[INDEX_NUM ] = '\0';
	char readcontent[BLOCK_SIZE];
	if (addrr[2] != -1) {//���ڶ�������
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
	//��ʱreadcontent������ϣ�offset�������
	//int first = 0;
	//if (addrr[2] == -1 && addrr[1] == -1 && addrr[0] == 0) {//first page first 20 bits is filename
	//	first = 1;
	//}
	if (size_con + offset > BLOCK_SIZE) {
		if (offset != 0) {
			getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&readcontent, BLOCK_SIZE);
		}
		memcpy(readcontent + offset, content, BLOCK_SIZE - offset);
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, readcontent, BLOCK_SIZE);//����д��cache
		inode.f_offset += (BLOCK_SIZE - offset);
		if (inode.f_offset >= inode.d_size) {//Ҫ�·���һ��block
			new_block_to_file(inode);
			inode.d_size = inode.f_offset;
			//����block,���Ҹ���dsize
		}
		size_con -= (BLOCK_SIZE - offset);
		memcpy(content, content + (BLOCK_SIZE - offset), size_con);//��ǰ���contentȥ��
		//inode.d_size += size_con > BLOCK_SIZE ? BLOCK_SIZE : size_con;
		goto START;
	}
	else {
		getblk(blkno + FILE_POS, CacheBlock::BLOCK, (char*)&readcontent, BLOCK_SIZE);
		memcpy(readcontent + offset, content,size_con);
		cachelist.writeCache(blkno + FILE_POS, CacheBlock::BLOCK, readcontent, offset + size_con);//����д��cache
		//cout << "w_size:" << offset % (BLOCK_SIZE - first * FILENAME_LENGTH)<<" "<< size_con <<" "<< first * FILENAME_LENGTH << endl;
		inode.f_offset += size_con;
		inode.d_size = inode.d_size < inode.f_offset ? inode.f_offset : inode.d_size;
		inode.d_atime = time(NULL);//������ʱ��
	}


	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inodeд��cache
	cout << "��ǰ�ļ�ָ�룺" << old << "��д���ֽ�����" << size << endl;
	
}

string fread(int fdnum, int size) {
	string res="";
	if (fdnum >= OPEN_FILE_NUM || fdnum < 0) {
		cout << "�ļ�����Ƿ���" << endl;
		//throw(NAME_LONG);
		return "";
	}
	if (open_file[fdnum] == -1) {
		cout << "�������ļ����" << fdnum << endl;
		//throw(FILE_NOT_EXIST);
		return "";
	}
	DiskInode inode;//��file��inode
	getblk(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);
	//��ʱinode load���
	if (inode.f_offset >= inode.d_size) {//������
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
	vector<int> addrr = off_to_block(offset);//offset��λ��block
	//cout << addrr[2] << " " << addrr[1] << " " << addrr[0] << endl;
	int blkno;
	unsigned int stack[INDEX_NUM + 1];
	stack[INDEX_NUM] = '\0';
	char readcontent[BLOCK_SIZE];
	if (addrr[2] != -1) {//���ڶ�������
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
	//��ʱreadcontent�������
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
		inode.d_atime = time(NULL);//������ʱ��
	}
	cachelist.writeCache(open_file[fdnum] + INODE_POS, CacheBlock::INODE, (char*)&inode, sizeof inode);//inodeд��cache
	
	cout<< "��ǰ�ļ�ָ�룺" << old << "���ļ���С��" << inode.d_size << "������ֽ�����" << rsize << "\n�����";
	//string tips = "��ǰ�ļ�ָ�룺" + to_string(old) + "���ļ���С��" + to_string(inode.d_size) + "������ֽ�����" + to_string(size) + "\n�����";
	return res;
}

void exit() {
	cachelist.writeBack();//�໺��д��
	//������ȫ�ֱ���д�أ�cachelist����Ҫд�أ��������ڴ��
	fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
	//���û�д��ļ��������ʾ��Ϣ��throw����
	if (!fd.is_open()) {
		cout << "�޷����ļ���" << DISK_NAME << endl;
		throw(CANT_OPEN_FILE);
	}
	//д��superblock
	fd.seekg(SUPERBLOCK_POS * BLOCK_SIZE, ios::beg);
	fd.write((char*)&superblock, sizeof(superblock));
	//д��directory
	int dic_inode_num = directory.d_inodenumber[0];//dictory��inode
	int dic_block_num;
	DiskInode dic_Inode;
	fd.seekg(INODE_POS * BLOCK_SIZE + dic_inode_num * INODE_SIZE, ios::beg);
	fd.read((char*)&dic_Inode, sizeof(dic_Inode));
	dic_block_num = dic_Inode.d_addr[0];
	fd.seekg((FILE_POS + dic_block_num) * BLOCK_SIZE, ios::beg);
	fd.write((char*)&directory, sizeof(directory));
}