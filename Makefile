ctable.dll : ctable.c
	gcc -g -Wall -o $@ --shared $^ -I/usr/local/include -L/usr/local/bin -llua53

clean :
	rm ctable.dll
