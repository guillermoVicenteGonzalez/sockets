programa: clientcp servidor

clientcp: clientcp.c
	gcc clientcp.c -o clientcp

servidor: servidor.c
	gcc servidor.c -o servidor

test: test.c
	gcc test.c -o test