shell: shell.c
	gcc -o shell shell.c

clean:
	rm -f *.o *.x core.*
