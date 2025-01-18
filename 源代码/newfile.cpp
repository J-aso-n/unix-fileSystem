#include "head.h"

void newfile() {
	string filename = "a.txt";
	fstream fd(filename, ios::out | ios::binary);
	if (!fd.is_open()) {
		cout << "无法打开文件 " << filename << endl;
		//throw(CANT_OPEN_FILE);
	}

	string content = "";
	for (int i = 0; i < 800; i++) {
		string mid = to_string(i);
		content += mid;
		content += " ";
		if (content.length() >= 800) break;
	}
	cout << content.length() << " " << content << endl;
	fd.write(content.c_str(), content.length());

	fd.close();
}


/*
y
fcreate 123
fopen 123
fwrite 0 test.exe
exit

fsave
fdelete 123

y
fcreate 123
fopen 123
fwrite 0 Report.pdf
exit

fsave
fdelete 123

y
fcreate 123
fopen 123
fwrite 0 a.txt
fread 0 std 500
fread 0 std 500
fseek 0 500 beg
fread 0 b.txt 500

fread 0 b.txt
fwrite 0 a.txt


y
mkdir bin
mkdir etc
mkdir home
mkdir dev
cd home
mkdir texts
mkdir reports
mkdir photos
cd ..

cd home
cd texts
fcreate readme

n
cd home
cd texts
fopen Jerry
fseek 0 500 beg
fread 0 std 500



*/