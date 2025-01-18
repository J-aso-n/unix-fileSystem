#include "head.h"
#include <sstream>

//全局变量
Directory directory;
CacheList cachelist;
SuperBlock superblock;
int open_file[OPEN_FILE_NUM];//打开文件的inode编号

void usage()
{
	cout << "***************************************************************************************" << endl
		<< "*                                                                                     *" << endl
		<< "*                                   Unix文件系统模拟                                  *" << endl
		<< "*                                                                                     *" << endl
		<< "* [usage]:                                                                            *" << endl
		<< "* help\t\t\t\t[功能]:命令提示                                       *" << endl
		<< "* fformat\t\t\t[功能]:格式化文件系统                                 *" << endl
		<< "* ls\t\t\t\t[功能]:查看当前目录内容                               *" << endl
		<< "* mkdir <dirname>\t\t[功能]:新建文件夹                                     *" << endl
		<< "* cd <dirname>\t\t\t[功能]:进入目录                                       *" << endl
		<< "* fcreate <filename>\t\t[功能]:新建文件filename                               *" << endl
		<< "* fopen <filename>\t\t[功能]:打开文件filename                               *" << endl
		<< "* fwrite <fdnum> <infile> <size>[功能]:从文件infile写入fdnum文件size字节              *" << endl
		<< "* fwrite <fdnum> std \t\t[功能]:从屏幕写入fdnum文件size字节                    *" << endl
		<< "* fread <fdnum> <outfile> <size>[功能]:从fdnum文件读取size字节，输出到outfile         *" << endl
		<< "* fread <fdnum> std <size>\t[功能]:从fdnum文件读取size字节，输出到屏幕            *" << endl
		<< "* fseek <fdnum> <step> begin\t[功能]:fdnum文件指针从开头偏移step                    *" << endl
		<< "* fseek <fdnum> <step> cur\t[功能]:fdnum文件指针从现有位置偏移step                *" << endl
		<< "* fseek <fdnum> <step> end\t[功能]:fdnum文件指针从末尾偏移step                    *" << endl
		<< "* fflag <fdnum> read\t\t[功能]:将fdnum文件权限更改为只读                      *" << endl
		<< "* fflag <fdnum> write\t\t[功能]:将fdnum文件权限更改为可读写                    *" << endl
		<< "* fclose <fdnum>\t\t[功能]:关闭文件句柄为fdnum的文件                      *" << endl
		<< "* fdelete <filename>\t\t[功能]:删除文件文件名为filename的文件或者文件夹       *" << endl
		<< "* exit \t\t\t\t[功能]:退出文件系统                                   *" << endl
		<< "* fsave\t\t\t\t[功能]:保存文件系统，cache写回（测试命令）            *" << endl
		<< "* 注意：退出文件系统要使用exit命令！不能直接退出！                                    *" << endl
		<< "***************************************************************************************" << endl;


}

int main()
{
	//init();
	usage();
	string line,command;
	char ch;
	while (1) {
		cout << "是否对文件系统进行初始化？[y/n]" << endl;
		if ((ch = getchar()) == 'y') {
			init();
			break;
		}
		else if (ch == 'n') {
			fstream fd(DISK_NAME, ios::out | ios::in | ios::binary);
			//如果没有打开文件则输出提示信息并throw错误
			if (!fd.is_open()) {
				cout << "无法打开文件卷" << DISK_NAME << endl;
				throw(CANT_OPEN_FILE);
			}
			//几个全局变量的初始化
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

		//格式化
		if (command == "fformat") {
			init();
			cout << "格式化完毕！" << endl;
		}
		else if (command == "exit") {
			exit();
			cout << "退出文件系统！" << endl;
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
			if (filename == "std") {//屏幕输入
				cout << "输入：";
				string content;
				getline(cin, content);
				int size = content.size();
				fwrite(fdnum, content.c_str(), size);
			}
			else if (filename == "") {
				cout << "非法文件名" << endl;
			}
			else {//文件输入
				//newfile();
				int size = -1;
				in >> size;
				fstream fd(filename, ios::in | ios::binary);
				//如果没有打开文件则输出提示信息并throw错误
				if (!fd.is_open()) {
					cout << "无法打开文件 " << filename << endl;
					//throw(CANT_OPEN_FILE);
					continue;
				}
				if (size < 0) {
					fd.seekg(0, ios::end); // 定位到文件末尾
					size = (int)fd.tellg(); // 获取文件长度
				}
				fd.seekg(0, ios::beg); // 定位到文件开头
				string content;
				// 读取文件内容到字符串
				content.resize(size); // 调整字符串大小以容纳文件内容
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
			else {//输出到文件中
				fstream fd(filename, ios::out | ios::binary);
				if (!fd.is_open()) {
					cout << "无法打开文件 " << filename << endl;
					//throw(CANT_OPEN_FILE);
				}
				fd.write(content.c_str(), content.length());
				fd.close();
				cout << "成功输出到 " << filename << " 中！" << endl;
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
			cout << "不存在此命令！" << endl
				<< "输入help查看命令集" << endl;
		}
	}
	return 0;
}