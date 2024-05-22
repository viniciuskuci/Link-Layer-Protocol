compile: app
app: app.c
	gcc -o app app.c stm.c byte_stuff.c linklayer.c
clean:
	rm -f app
cable:
	gcc -o cable cable.c
