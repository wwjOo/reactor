server_src= server.cpp inetaddress.cpp socket.cpp epoll.cpp channel.cpp eventloop.cpp \
			tcpserver.cpp acceptor.cpp connection.cpp buffer.cpp echoserver.cpp threadpool.cpp \
			timestamp.cpp

target:server-test

#Make通过比较源代码文件的时间戳和目标文件的时间戳来判断代码是否被修改过，进而判断是否需要重新编译
server-test:${server_src} 
	g++ -g -pthread -o server-test ${server_src}

clean:
	rm -rf server-test core*

server:
	./server-test 10.1.20.9 5005
