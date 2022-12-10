programa: clientcp servidor clientudp cliente

cliente: cliente.c
	gcc cliente.c -o cliente

clientudp: clientudp.c
	gcc clientudp.c -o clientudp

clientcp: clientcp.c
	gcc clientcp.c -o clientcp

servidor: servidor.c
	gcc servidor.c -o servidor

test: test.c
	gcc test.c -o test
