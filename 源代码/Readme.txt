一、程序形式：cmd形式
	开发平台: Windows 11
	开发工具：VS2022 x86
	采用命令行交互方法运行，800.txt、Readme.txt、report.pdf、photo.png是输入源文件，请不要删除。
二·、命令说明
* [usage]:                                                                            
* help                         				[功能]:命令提示                                       
* fformat                      				[功能]:格式化文件系统                                 
* ls                            				[功能]:查看当前目录内容                               
* mkdir <dirname>              			[功能]:新建文件夹                                     
* cd <dirname>                  		  	[功能]:进入目录                                       
* fcreate <filename>            		        [功能]:新建文件filename                               
* fopen <filename>               		[功能]:打开文件filename                               
* fwrite <fdnum> <infile> <size>	[功能]:从文件infile写入fdnum文件size字节              
* fwrite <fdnum> std             		[功能]:从屏幕写入fdnum文件size字节                    
* fread <fdnum> <outfile> <size>     [功能]:从fdnum文件读取size字节，输出到outfile         
* fread <fdnum> std <size> 		[功能]:从fdnum文件读取size字节，输出到屏幕            
* fseek <fdnum> <step> begin    	[功能]:fdnum文件指针从开头偏移step                    
* fseek <fdnum> <step> cur      		[功能]:fdnum文件指针从现有位置偏移step                
* fseek <fdnum> <step> end      	[功能]:fdnum文件指针从末尾偏移step                    
* fflag <fdnum> read           	 		[功能]:将fdnum文件权限更改为只读                      
* fflag <fdnum> write           		[功能]:将fdnum文件权限更改为可读写                    
* fclose <fdnum>                			[功能]:关闭文件句柄为fdnum的文件                      
* fdelete <filename>            			[功能]:删除文件文件名为filename的文件或者文件夹       
* exit                         	 			[功能]:退出文件系统                                   
* fsave                         				[功能]:保存文件系统，cache写回（测试命令）            
* 注意：退出文件系统要使用exit命令！不能直接退出！                                    