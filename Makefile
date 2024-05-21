compile: read write
read: readnoncanonical.c
	gcc -o read readnoncanonical.c stm.c byte_stuff.c
write: writenoncanonical.c 
	gcc -o write writenoncanonical.c stm.c byte_stuff.c
clean:
	rm -f read write
cable:
	gcc -o cable cable.c
