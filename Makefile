all:
	g++ -lcurl -shared -fPIC -Wall -g mysql_growthforcast.cc -o mysql_growthforcast.so
