obj=server.o

server:$(obj)
	g++ -o server $(obj)
server.o:server.cpp ../util.h
	g++ -c server.cpp

clean:
	rm server $(obj)
