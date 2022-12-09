# sockets

## Autores:
Guillermo Vicente González
Jorge Prieto de la Cruz

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


 - [x] Fragmentar data si pesa mas de 516 (simplemente fgets tambuffer??) ?????

 - [ ] cliente
 	 - [x] correcta invocacion (tcp + udp en uno) (revisar)

	 - [x] cliente tcp
	 	- [x] funcionalidad basica
	 	- [x] control de errores (revisar)
	 	- [x] log de cliente (joder que coñazo)

	 - [ ] cliente udp
	 	- [x] funcionalidad basica
	 	- [ ] control de errores (revisar)
	 	- [x] No llega quit
	 	- [x] log de cliente? (joder que coñazo)
 
 - [ ] servidor
 	- [ ] servidor tcp
 	 	- [x] funcionalidad basica
 			- [x] bucle de recepcion
 			- [x] tratamiento completo de cadenas
 			- [x] secuencia correcta (revisar)
 		- [x] concurrencia
 			- [x] socket de escucha
 			- [x] proceso hijo
 		- [x] fichero de log
 			- [x] funcionalidad basica
 			- [x] cada peticion en una linea 
 				- salen en dos lineas. Send y from o send y to(aceptamos pulpo)
 			- [x] fechas
 		- [ ] comprobar que el host de HELO es mi host

 	- [ ] servidor udp
 		- [x] funcionalidad basica
 			- [x] bucle de recepcion
 			- [x] tratamiento completo de cadenas
 			- [x] alarmas e intentos
 			- [x] secuencia correcta (revisar)
 		- [x] concurrencia
 			- [x] socket de escucha
 			- [x] proceso hijo
 			- [x] salida ordenada (revisar)
 		- [x] fichero de log
 			- [x] funcionalidad basica
 			- [x] cada peticion en una linea 
 				- salen en dos lineas. Send y from o send y to(aceptamos pulpo)
 			- [x] fechas
 		- [ ] comprobar que el host de HELO es mi host
 		
 - [ ] pruebas en nogal
 - [ ] comentar y limpiar el codigo 

