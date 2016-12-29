taskscheduler: scheduler.o
	clang++-3.5 scheduler.o sql.o crekub_lib.o -o taskscheduler /usr/lib/x86_64-linux-gnu/libmysqlclient.so.18 /usr/lib/x86_64-linux-gnu/libidn.so.11
	rm scheduler.o
	rm -rf .logs
	mkdir .logs
	mysql -u scc --password=ZQLf-0.4 < init.sql
scheduler.o:
	clang++-3.5 -Wall -std=c++11 -c scheduler.cpp 