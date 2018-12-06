TP2: Procesos de usuario
========================

env_alloc
---------
Los primeros 5 identificadores (en base hexadecimal) serán:
	- 0x400
	- 0x401
	- 0x402
	- 0x403
	- 0x404

En este escenario, el único ambiente libre será el que se encuentra en envs[630], cuyo ID en base hexadecimal es 276. Al llamar a env_alloc, se le asignará el id 676. Una vez que sea destruido, nuevamente el único ambiente libre será el que se encuentra en envs[630], y al volver a lanzar el proceso volverá a tener el mismo identificador.

env_init_percpu
---------------
La instrucción lgdt() se encarga de cargar en el registro GDTR la base address de la Global Descriptor Table (GDT) y su limit (en bytes). Es decir, la instrucción lgdt() carga en el registro GDTR un total de 48 bits, o bien 1.5 bytes.


env_pop_tf
----------
Tras el primer movl de la función, en %esp se guarda la dirección de memoria que apunta al struct TrapFrame tf que se recibe por parámetro. Ésta dirección corresponde particularmente al puntero al struct PushRegs que guarda el estado de los registros del environment.

Justo antes de la instrucción iret, en %esp se encuentra el Instruction Pointer que se cargará en el registro %eip y es la dirección de memoria a donde saltará la ejecución. En 8(%esp) se encuentra el Trap Number (trapno).

La CPU puede determinar si hay un cambio de ring según el valor de los privilegios del Code Segment (3 para privilegios de usuario, 0 para privilegios de Kernel) y los valores de las entradas en la Global Descriptor Table.

gdb_hello
---------
(qemu) info registers
EAX=003bc000 EBX=f01c0000 ECX=f03bc000 EDX=00000239
ESI=00010094 EDI=00000000 EBP=f0118fd8 ESP=f0118fbc
EIP=f0102efa EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

(gdb) p tf
$1 = (struct Trapframe *) 0xf01c0000
(gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17
(gdb) x/17x tf
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023
-

(gdb) si 4
=> 0xf0102f03 <env_pop_tf+9>:	popa   
0xf0102f03	523		asm volatile("\tmovl %0,%%esp\n"
(gdb) x/17x $sp
0xf01c0000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c0020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c0030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c0040:	0x00000023

-------> Los contenidos son los mismos!
(7) Los primeros ocho valores, que están en cero, son los valores de los registros correspondientes al proceso. Más precisamente, los registros que forman parte del struct PushRegs (edi, esi, ebp, oesp, ebx, edx, ecx, eax).
Luego, encontramos el valor 0x023 que corresponde al ES del TrapFrame y se repite para el DS.
Trapno y tf_err valen 0x0 (lo cual es consistente).
En la dirección de memoria 0xf01c0030 encontramos en primer lugar el Instruction Pointer, y luego el registro CS (y el padding 3). Seguido de ellos, se encuentra EFLAGS (cuyo valor es 0).
Por último se encuentran el Stack Pointer y en la dirección 0xf01c0040, encontramos el SS (y el último padding).


(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c0030
EIP=f0102f09 EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]

(gdb) p $pc
$3 = (void (*)()) 0x800020
(gdb) p $eip
$4 = (void (*)()) 0x800020
-----> carga de símbolos:
(gdb) p $pc
$5 = (void (*)()) 0x800020 <_start>
(gdb) p $eip
$6 = (void (*)()) 0x800020 <_start>

(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]

después del punto 10 esto tira info registers:

(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e05b EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00


(10) Justo luego de ejecutarse la instrucción int $0x30 el context switch ya se realizó. El kernel entonces ejecuta la macro TRAPHANDLER_NOEC correspondiente al número 48 (0x30), que luego lleva la ejecución al símbolo alltraps (de trapentry.S). Luego, salta a la función trap() que va a saltar a la función trap_dispatch() a donde finalmente se realiza la llamada a la syscall que a través de un switch() termina por ejecutar la syscall apropiada utilizando el parámetro correspondiente.


kern_idt
--------
Las macros TRAPHANDLER y TRAPHANDLER_NOEC tienen como finalidad realizar un salto hacia el símbolo "alltraps" del archivo trapentry.S. Antes, sin embargo, se diferencian en que la macro TRAPHANDLER recibe (y pushea al stack) un código de error. Por el contrario, TRAPHANDLER_NOEC se utiliza para aquellas excepciones, interrupciones o traps que no poseen un código de error. Para mantener el orden en el Stack, ésta macro pushea un 0. Si usáramos solamente la primera tendríamos una inconsistencia para los casos que no tienen código de error.

A la hora de invocar handlers, el segundo parámetro (istrap) sirve para definir el evento como una excepción o una interrupción.
Durante un syscall, según el valor de istrap se cargan los bits de segmento para comunicarle al procesador si se trata de una excepción (Trap Gate: STS_TG32) o una interrupción (Interrupt Gate: STS_IG32). En el caso de una syscall, el evento es asincrónico, por lo que se trata de una interrupción y por eso seteamos el valor de istrap en 0. 

Al correr el archivo softint.c, se genera la excepción 13 (General Protection). El código intenta generar una interrupción 14 (Page Fault), pero al tratar de hacerlo en el modo usuario el Kernel lo identifica como una vulneración y se lo impide: únicamente se puede levantar una interrupción 14 con privilegios de Kernel. La idea de este mecanismo es evitar que el usuario genere interrupciones que no le corresponden, sin perjudicar al modo de generar aquellas que sí le corresponde.


user_evilhello
--------------
La versión evilhello directamente trata de imprimir (como char*) lo que se encuentra en una dirección de memoria (que corresponde al entry point del kernel). En la versión user_evilhello se trata de imprimir una variable guardada en el stack del space address.
No hay diferencia en el comportamiento de las dos ejecuciones. Esto se debe a que ambas versiones tratan de imprimir "un char*" almacenado en una dirección que supera ULIM y por lo tanto corresponde a direcciones mapeadas al Kernel. El usuario no tiene la posibilidad de hacer esto para protección del Kernel.