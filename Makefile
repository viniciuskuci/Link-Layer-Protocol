compile: read write
read: readnoncanonical.c
	gcc -o read readnoncanonical.c reciever_sm.c
write: writenoncanonical.c 
	gcc -o write writenoncanonical.c sender_sm.c
clean:
	rm -f read write
cable:
	gcc -o cable cable.c
