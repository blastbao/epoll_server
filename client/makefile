obj=client.o

client:$(obj)
	g++ -o client $(obj)
client.o:client.c ../util.h
	g++ -c client.c

clean:
	rm client $(obj)
