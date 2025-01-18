#include "head.h"
#include <sstream>

//ȫ�ֱ���
Directory directory;
CacheList cachelist;
SuperBlock superblock;
int open_file[OPEN_FILE_NUM];//���ļ���inode���

void usage()
{
	cout << "***************************************************************************************" << endl
		<< "*                                                                                     *" << endl
		<< "*                                   Unix�ļ�ϵͳģ��                                  *" << endl
		<< "*                                                                                     *" << endl
		<< "* [usage]:                                                                            *" << endl
		<< "* help\t\t\t\t[����]:������ʾ                                       *" << endl
		<< "* fformat\t\t\t[����]:��ʽ���ļ�ϵͳ                                 *" << endl
		<< "* ls\t\t\t\t[����]:�鿴��ǰĿ¼����                               *" << endl
		<< "* mkdir <dirname>\t\t[����]:�½��ļ���                                     *" << endl
		<< "* cd <dirname>\t\t\t[����]:����Ŀ¼                                       *" << endl
		<< "* fcreate <filename>\t\t[����]:�½��ļ�filename                               *" << endl
		<< "* fopen <filename>\t\t[����]:���ļ�filename                               *" << endl
		<< "* fwrite <fdnum> <infile> <size>[����]:���ļ�infileд��fdnum�ļ�size�ֽ�              *" << endl
		<< "* fwrite <fdnum> std \t\t[����]:����Ļд��fdnum�ļ�size�ֽ�                    *" << endl
		<< "* fread <fdnum> <outfile> <size>[����]:��fdnum�ļ���ȡsize�ֽڣ������outfile         *" << endl
		<< "* fread <fdnum> std <size>\t[����]:��fdnum�ļ���ȡsize�ֽڣ��������Ļ            *" << endl
		<< "* fseek <fdnum> <step> begin\t[����]:fdnum�ļ�ָ��ӿ�ͷƫ��step                    *" << endl
		<< "* fseek <fdnum> <step> cur\t[����]:fdnum�ļ�ָ�������λ��ƫ��step                *" << endl
		<< "* fseek <fdnum> <step> end\t[����]:fdnum�ļ�ָ���ĩβƫ��step                    *" << endl
		<< "* fflag <fdnum> read\t\t[����]:��fdnum�ļ�Ȩ�޸���Ϊֻ��                      *" << endl
		<< "* fflag <fdnum> write\t\t[����]:��fdnum�ļ�Ȩ�޸���Ϊ�ɶ�д                    *" << endl
		<< "* fclose <fdnum>\t\t[����]:�ر��ļ����Ϊfdnum���ļ�                      *" << endl
		<< "* fdelete <filename>\t\t[����]:ɾ���ļ��ļ���Ϊfilename���ļ������ļ���       *" << endl
		<< "* exit \t\t\t\t[����]:�˳��ļ�ϵͳ                                   *" << endl
		<< "* fsave\t\t\t\t[����]:�����ļ�ϵͳ��cacheд�أ��������            *" << endl
		<< "* ע�⣺�˳��ļ�ϵͳҪʹ��exit�������ֱ���˳���                                    *" << endl
		<< "***************************************************************************************" << endl;


}

int main()
{
	//init();
	usage();
	string line,command;
	char ch;
	while (1) {
		cout << "�Ƿ���ļ�ϵͳ���г�ʼ����[y/n]" << endl;
		if ((ch = getchar()) == 'y') {
			init();
			break;
		}
		else if (ch == 'n') {
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//���û�д��ļ��������ʾ��Ϣ��throw����
			if (!fd.is_open()) {
				cout << "�޷����ļ���" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			//����ȫ�ֱ����ĳ�ʼ��
			fd.seekg(FILE_POS * BLOCK_SIZE, ios::beg);
			fd.read((char*)&directory, sizeof(directory));
			fd.seekg(SUPERBLOCK_POS * BLOCK_SIZE, ios::beg);
			fd.read((char*)&superblock, sizeof(superblock));
			for (int i = 0; i < OPEN_FILE_NUM; i++) {
				open_file[i] = -1;
			}
			break;
		}
		while (ch != '\n' && getchar() != '\n');
	}
	while (ch != '\n' && getchar() != '\n');

	while (1) {
		cout <<endl<< "[@ "<<directory.name<<" ]$ ";
		getline(cin, line);
		if (line.size() == 0)
			continue;
		stringstream in(line);
		in >> command;

		//��ʽ��
		if (command == "fformat") {
			init();
			cout << "��ʽ����ϣ�" << endl;
		}
		else if (command == "exit") {
			exit();
			cout << "�˳��ļ�ϵͳ��" << endl;
			break;
		}
		else if (command == "help") {
			usage();
		}
		else if (command == "ls") {
			ls();
		}
		else if (command == "mkdir") {
			string filename;
			in >> filename;
			mkdir(filename.c_str());
		}
		else if (command == "cd") {
			string filename;
			in >> filename;
			cd(filename.c_str());
		}
		else if (command == "fcreate") {
			string filename;
			in >> filename;
			fcreate(filename.c_str());
		}
		else if (command == "fopen") {
			string filename;
			in >> filename;
			fopen(filename.c_str());
		}
		else if (command == "fclose") {
			int fdnum;
			in >> fdnum;
			fclose(fdnum);
		}
		else if (command == "fseek") {
			int fdnum, dis;
			string type;
			in >> fdnum >> dis >> type;
			if (type == "begin" || type == "BEGIN" || type == "beg" || type == "BEG") {
				fseek(fdnum, dis, BEG);
			}
			else if (type == "cur" || type == "CUR") {
				fseek(fdnum, dis, CUR);
			}
			else if (type == "end" || type == "END") {
				fseek(fdnum, dis, END);
			}
			else {
				goto ERROR;
			}
		}
		else if (command == "fsave") {
			exit();
		}
		else if (command == "fwrite") {
			int fdnum;
			string filename;
			in >> fdnum >> filename;
			if (filename == "std") {//��Ļ����
				cout << "���룺";
				string content;
				getline(cin, content);
				int size = content.size();
				fwrite(fdnum, content.c_str(), size);
			}
			else if (filename == "") {
				cout << "�Ƿ��ļ���" << endl;
			}
			else {//�ļ�����
				//newfile();
				int size = -1;
				in >> size;
				fstream fd(filename, ios::in | ios::binary);
				//���û�д��ļ��������ʾ��Ϣ��throw����
				if (!fd.is_open()) {
					cout << "�޷����ļ� " << filename << endl;
					//throw(CANT_OPEN_FILE);
					continue;
				}
				if (size < 0) {
					fd.seekg(0, ios::end); // ��λ���ļ�ĩβ
					size = (int)fd.tellg(); // ��ȡ�ļ�����
				}
				fd.seekg(0, ios::beg); // ��λ���ļ���ͷ
				string content;
				// ��ȡ�ļ����ݵ��ַ���
				content.resize(size); // �����ַ�����С�������ļ�����
				fd.read(&content[0], size);
				fd.close();

				fwrite(fdnum, content.c_str(), size);
			}
		}
		else if (command == "fread") {
			int fdnum, size;
			string filename;
			in >> fdnum >>filename >> size;
			string content = fread(fdnum, size);
			if (filename == "std") {
				int i = 0;
				for (auto c : content) {
					if (i % 80 == 0)
						cout << endl;
					cout << c;
					i++;
				}
				cout << endl;
			}
			else {//������ļ���
				fstream fd(filename, ios::out | ios::binary);
				if (!fd.is_open()) {
					cout << "�޷����ļ� " << filename << endl;
					//throw(CANT_OPEN_FILE);
				}
				fd.write(content.c_str(), content.length());
				fd.close();
				cout << "�ɹ������ " << filename << " �У�" << endl;
			}
		}
		else if (command == "fdelete") {
			string filename;
			in >> filename;
			fdelete(filename.c_str());
		}
		else if (command == "fflag") {
			int fdnum;
			string type;
			in >> fdnum >> type;
			fflag(fdnum, type);
		}
		else {
			ERROR:
			cout << "�����ڴ����" << endl
				<< "����help�鿴���" << endl;
		}
	}
	return 0;
}