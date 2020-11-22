all: client

clean:
	@rm -rf *.o
	@rm -rf client

client: client.o
	gcc -o client $^

main.o: client.c
	gcc -c -o client.o client.c