=====================================
=			特别说明				=
=====================================
1.在src/common/log.h 中DEBUG默认设置为0，即默认不输出log等debug信息。如果需要查看log信息，可以将DEBUG设为1，生成的log在srouted可执行文件所在目录下，每个node会有以其名字命名的log，有详细信息。
2.在开启log模式下，如果可能会出现无法通过测试脚本的情况，原因是log信息过多（运行sample_final.sh脚本每个node.log会输出3w行左右log信息）影响性能，使得在daemon向脚本返回信息前脚本已经进行判断，在相应脚本的eval_test前sleep(1)即可通过
3.本代码已在多台ubuntu12.04虚拟机上通过测试：
 Intel® Core™ i5 CPU M 480 @ 2.67GHz × 4 DEBUG = 0 测试通过
 Intel® Core™ i7-2630QM CPU @ 2.00GHz × 8  1G ram DEBUG = 1 测试通过
 如有问题，可以联系10302010051@fudan.edu.cn ^_^
=====================================
=		  设计思路说明              =
=====================================
该project使用select设置非阻塞来实现I/O多路复用，同时对TCP和UDP的socket进行监听并进行处理。每次循环开始计算当前时间处理重发和过期等操作，然后使用select接收各个socket的信息进行判断和处理。详细设计流程图参见“计算机网络pj2设计思路.png”


======================================
=		   文件及功能说明            =
======================================

-finalcheckpoint 
测试文件，当DEBUG = 1时，会在该文件夹下生成.log文件，分别记录了每个node的收发和处理的详细信息
-sample-final.sh
自动编译src/daemon目录下的源文件生成srouted并拷贝到finalcheckpoint文件夹下，自动执行sample_final_test.rb脚本.只需在当前目录下运行sh sample-final.sh即可
-script.sh
功能同上，执行测试脚本为script.sh
-test_daemon.sh
checkpoint1中测试用脚本，自动编译src/daemon目录下的源文件生成srouted并拷贝到test_checkpoint1文件夹下，自动执行test_rdaemon.rb脚本.只需在当前目录下运行sh test_daemon.sh即可
-test_server.sh
checkpoint1中测试用脚本，自动编译src/server目录下的源文件生成sircd并拷贝到test_checkpoint1文件夹下，自动执行checkpoint1.rb脚本.只需在当前目录下运行sh test_server.sh即可

-src
	-common 需要被公用的代码文件夹
		-csapp.c （support code）
		-rtlib.c  (support code)
		-log.c    生成log相关代码，debug使用。在
				  DEBUG=1时开启
		-socket.c socket连接相关代码
		-util.c   server/daemon初始化相关代码，
				  字符串处理相关代码

	-daemon daemon相关代码
		-srouted.c daemon运行入口，核心循环,处理serv-
				   er cmd，处理收到的LSA和发送重传
		-channel_cache.c 
				   定义channel_cache和list结构，处理
				   channel_cache_list的增删改查相关代
				   码
		-user_cache.c
				   定义user_cache和lis结构，处理user_
				   cache_list的增删改查相关代码
		-lsa_list.c
				   定义从其他daemon收到的LSA存储结构，
				   处理其list的增删改查相关代码
		-routing_table.c
				   计算和生成最短路径树相关代码
		-rtgrading.c (support code)
		-udp.c 	   udpserver启动和发送方法
		-wait_ack_list.c
				   定义需要等待ack的lsa结构，及相应的
				   增删方法

	-server checkpoint1中server相关代码文件夹，final中
			不再使用



