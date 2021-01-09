install:
	gcc -c src/header_list.c src/req_parser.c src/sys_utils.c
	g++ -Wall -Wextra -Werror -Wpedantic -g -pthread -o server req_parser.o header_list.o sys_utils.o src/*.cpp
	rm -f req_parser.o header_list.o sys_utils.o
uninstall:
	rm -f server
reinstall:
	make uninstall
	make install