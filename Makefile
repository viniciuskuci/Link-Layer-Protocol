compile: read write
read: readnoncanonical.c
	gcc -o app app.c linklayer.c stm.c byte_stuff.c
clean:
	rm -f app

