programa: clientcp servidor clientudp

clientudp: clientudp.c
	gcc clientudp.c -o clientudp

clientcp: clientcp.c
	gcc clientcp.c -o clientcp

servidor: servidor.c
	gcc servidor.c -o servidor

test: test.c
	gcc test.c -o test
