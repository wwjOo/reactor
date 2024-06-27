client_src= client.cpp file.cpp buffer.cpp

target:client-test

#Make通过比较源代码文件的时间戳和目标文件的时间戳来判断代码是否被修改过，进而判断是否需要重新编译

client-test:${client_src}
	g++ -g -pthread -o client-test ${client_src}

./PHONY: clean client

clean:
	rm -rf client-test core*

client:
	./client-test 0.0.0.0 5005