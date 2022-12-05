# sockets

## modo de ejecucion

```bash
./servidor
./clientcp <nombreServer> <protocolo> <fichero.txt>
./clientudp <nombreHost> <nombreServer>

#ejemplos

#ejecutar servidor
./servidor

#maneras de ejecutar cliente tcp
./clientcp guille-manjaro TCP texto.txt
./clientcp $(hostname) TCP texto.txt
./clientcp localhost TCP texto.txt

#maneras de ejecutar clienteudp
./clientudp localhost localhost
./clientudp $(hostname) $(hostname)
```

## Cosas por hacer

 - [ ] cliente
 	 - [ ] correcta invocacion (tcp + udp en uno)

	 - [x] cliente tcp
	 	- [x] funcionalidad basica
	 	- [x] control de errores (revisar)

	 - [ ] cliente udp
	 	- [x] funcionalidad basica
	 	- [ ] control de errores (revisar)
 
 - [ ] servidor
 	- [ ] servidor tcp
 	 	- [x] funcionalidad basica
 		- [x] concurrencia
 			- [x] socket de escucha
 			- [x] proceso hijo
 		- [ ] fichero de log
 			- [x] funcionalidad basica
 			- [ ] cada peticion en una linea
 			- [ ] fechas

 	- [ ] servidor udp
 		- [ ] funcionalidad basica
 			- [x] bucle de recepcion
 			- [ ] tratamiento completo de cadenas
 			- [ ] alarmas e intentos
 		- [ ] concurrencia
 			- [x] socket de escucha
 			- [x] proceso hijo
 			- [ ] salida ordenada
 		- [ ] fichero de log
 			- [ ] funcionalidad basica
 			- [ ] cada peticion en una linea
 			- [ ] fechas
