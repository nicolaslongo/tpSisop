TP3.md
======


static_assert:
-------------
La macro static_assert de JOS utiliza las comprobaciones regulares del compilador C para hacer un chequeo. Si a la macro se le pasa el valor de x, y este es FALSO (0), entonces generará un error que no se generará si el valor de x es VERDADERO, 1 o cualquier otro número.


env_return:
----------
Env_run() es la función encargada de llamar a env_pop_tf() que le cede el control al usuario.
Una vez en el userspace, el programa comienza a ejecutarse desde el símbolo "start", que se ocupa de cargar (o verificar que estén) los argumentos y pasa a ejecutarse la función libmain, en la que se invoca a umain y cuando ésta termina se llama a la función exit(). Exit llama a sys_env_destroy para "matar" al proceso.


En el trabajo práctico anterior (TP2), env_destroy() ejecutaba a env_free() y automáticamente llamaba al monitor() de Jos. En este nuevo TP, env_destroy() va a ejecutar sched_yield(), que se encargará de seguir ejecutando el resto de los procesos en estado RUNNABLE (o bien, cederle el control al monitor() de Jos).


sys_yield:
---------

nicolas@nicolas-PC:~/sisop$ make qemu-nox
+ cc kern/init.c
+ ld obj/kern/kernel
+ mk obj/kern/kernel.img
***
*** Use Ctrl-a x to exit qemu
***
qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp:127.0.0.1:26000 -D qemu.log -smp 1  -d guest_errors
6828 decimal is 15254 octal!
Physical memory: 131072K available, base = 640K, extended = 130432K
boot_alloc memory at f0278000
Next memory at f0279000
boot_alloc memory at f0279000
Next memory at f02b9000
npages: 32768
npages_basemem: 160
pages: f0279000
boot_alloc memory at f02b9000
Next memory at f02d8000
indice MPENTRY_PADDR: 7
indice npages_basemem: 160
boot_alloc memory at f02d8000
Next memory at f02d8000
pageinfo size: 8
f02b9000
med=728
boot_alloc memory at f02d8000
Next memory at f02d8000
check_page_free_list() succeeded!
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
boot_alloc memory at f02d8000
Next memory at f02d8000
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Hello, I am environment 00001002.
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 4.
All done in environment 00001001.
[00001001] exiting gracefully
[00001001] free env 00001001
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 

El código del archivo user/yield.c recorre un ciclo y llama a la función sys_yield() un total de 4 veces, devolviéndole el control al Kernel.
Al modificar i386_init() para crear tres ambientes que corran este programa, la salida obtenida es la esperada: los 3 ambientes se crean, se ejecutan en orden y van llamando por primera vez a sys_yield: primero lo hace el primer ambiente (env_id = 00001000), luego el segundo (env_id = 00001001) y por último el tercero (env_id = 00001002), todos ellos dejando el control casi inmediatamente.
Entonces el scheduler va a ir devolviéndole el control a cada uno en el mismo orden. Primero, volverá el primer ambiente a finalizar la primera iteración del ciclo, entrará en la segunda y devolverá la ejecución al kernel; éste se la entregará al segundo ambiente que finalizará la primera iteración, entrará en la segunda y devolverá la ejecución al kernel; etc.
Ésta salida evidencia que el Scheduler implementa el algoritmo de scheduling conocido como Round-Robin.


envid2env:
---------

En Jos, si un proceso llama sys_env_destroy(0), entonces éste buscará el environment correspondiente al id que recibió por parámetro (en este caso, el environment 0). Al comenzar la ejecución de sys_env_destroy() hay una comparación: si el parámetro recibido es 0, entonces se devolverá el environment actual (*curenv*).
En Linux, si un proceso llama a kill(0,9), el primer argumento (pid_t pid) al ser 0 especifica que se envía el mensaje signal (en este caso 9) a todos los procesos del "process group" del proceso que llamó al wrapper de la syscall.

De igual manera, en Jos, sys_get_env_destroy(-1) devolverá el error -E_BAD_ENV ya que un ambiente en Jos no puede tener id negativo
En Linux, la llamada kill(-1, 9) le envía la señal 9 a todos aquellos procesos a los que el proceso que invoco al wrapper TIENE PERMISO para enviarle la señal (a excepción del proceso 1, es decir init).


dumbfork:
--------

1. Si la página se reserva entre UTEXT y char* end[], entonces al llamar a dumbfork() se propagará la copia al proceso hijo (mediante duppage()).

2. 
El estado de solo lectura no se preserva. La función duppage setea los bits de presencia, usuario y escritura. Esto es necesario, para poder realizar la copia de una página a la otra.
	void* algunPuntero;
	pte_t pageTable = uvpt[PGNUM(algunPuntero)];
	if(*pageTable & PTE_W)
		return true;
	else
		return false;*


3. La función duppage() literalmente duplica una página que es del padre, y la copia en el proceso hijo. En primer lugar reserva una página nueva (utilizando la misma dirección virtual que le corresponde en el espacio de direcciones del padre), luego la mappea en la dirección UTEMP de su propio espacio de direcciones.
Por último, una vez ahí copia el contenido de la página en UTEMP de manera tal que queda copiado en el espacio de direcciones del proceso hijo. Por último, *desmappea* la página de UTEMP.

4.  Si el booleano fuera true, entonces debería controlar el valor de perm con una estructura del tipo condicional. De ésta manera, los permisos incluirían o no el PTE_W, respectivamente.

Si quisiéramos poder definir a la página como solo-lectura sin aumentar el número de llamadas al sistema, el algoritmo descripto en el punto anterior funcionaría.


5. La dirección addr corresponde al tope del stack. Se hace ROUNDDOWN() para obtener el valor de comienzo de la página en cuestión. Esto es antinatural porque el stack crece para abajo (desde la dirección más alta de la página a la menos alta)



multicore_init:
--------------

1. La línea de código mencionada carga en memoria baja (una dirección virtual V en la que está mappeada la dirección física MPENTRY_ADDR, es decir 0x7000) parte del código escrito en el archivo mpentry.S. Analogamente a lo que sucede con el bootloader en boot.S , las application processor (o AP) bootean en real mode pero con la ventaja de que podemos realizar un mapeo que nos simplifique la ejecución del código.

2. La variable global mpentry_kstack apunta al tope de la dirección del Kernel Stack de cada uno de los CPUs.
En kern/entry.S, código que es ejecutado por una CPU, se setea el stack pointer al final del código mismo. Esto solo podría realizarse desde mpentry.S si todos los CPUs compartieran el mismo stack, y esto no funcionaría ya que al recibir interrupciones en simultáneo se pisaría la información de cada uno de los CPUs.

3. 
nicolas@nicolas-PC:~/sisop$ make gdḅ 
~
(gdb) watch mpentry_kstack
Hardware watchpoint 1: mpentry_kstack
(gdb) c
Continuando.
Se asume que la arquitectura objetivo es i386
=> 0xf0100186 <boot_aps+127>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0x0
New value = (void *) 0xf0249000 <percpu_kstacks+65536>*
boot_aps () at kern/init.c:112
112			lapic_startap(c->cpu_id, PADDR(code));

(gdb) bt
#0  boot_aps () at kern/init.c:112
#1  0xf010020f in i386_init () at kern/init.c:55
#2  0xf0100047 in relocated () at kern/entry.S:88

(gdb) info threads
  Id   Target Id         Frame 
* 1    Thread 1 (CPU#0 [running]) boot_aps () at kern/init.c:112
  2    Thread 2 (CPU#1 [halted ]) 0x000fd412 in ?? ()
  3    Thread 3 (CPU#2 [halted ]) 0x000fd412 in ?? ()
  4    Thread 4 (CPU#3 [halted ]) 0x000fd412 in ?? ()

(gdb) c
Continuando.
=> 0xf0100186 <boot_aps+127>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf0249000 <percpu_kstacks+65536>
New value = (void *) 0xf0251000 <percpu_kstacks+98304>
boot_aps () at kern/init.c:112
112			lapic_startap(c->cpu_id, PADDR(code));
#
(gdb) info threads
  Id   Target Id         Frame 
  1    Thread 1 (CPU#0 [running]) boot_aps () at kern/init.c:112
  2    Thread 2 (CPU#1 [running]) 0xf010027f in mp_main () at kern/init.c:130
  3    Thread 3 (CPU#2 [halted ]) 0x000fd412 in ?? ()
  4    Thread 4 (CPU#3 [halted ]) 0x000fd412 in ?? ()
#
(gdb) thread 2
[Switching to thread 2 (Thread 2)]
#0  0xf010027f in mp_main () at kern/init.c:130
130		xchg(&thiscpu->cpu_status, CPU_STARTED); // tell boot_aps() we're up
#
(gdb) bt
#0  0xf010027f in mp_main () at kern/init.c:130
#1  0x00007060 in ?? ()
#
(gdb) p cpunum()
Could not fetch register "orig_eax"; remote failure reply 'E14'
1  // Deducido
#
(gdb) thread 1
[Switching to thread 1 (Thread 1)]
#0  boot_aps () at kern/init.c:112
112			lapic_startap(c->cpu_id, PADDR(code));
#
(gdb) p cpunum()
Could not fetch register "orig_eax"; remote failure reply 'E14'
0  // Deducido
#
(gdb) c
Continuando.
=> 0xf0100186 <boot_aps+127>:	mov    %esi,%ecx

Thread 1 hit Hardware watchpoint 1: mpentry_kstack

Old value = (void *) 0xf0251000 <percpu_kstacks+98304>
New value = (void *) 0xf0259000 <percpu_kstacks+131072>
boot_aps () at kern/init.c:112
112			lapic_startap(c->cpu_id, PADDR(code));

(gdb) info threads
  Id   Target Id         Frame 
* 1    Thread 1 (CPU#0 [running]) boot_aps () at kern/init.c:112
  2    Thread 2 (CPU#1 [running]) 0xf010027f in mp_main () at kern/init.c:130
  3    Thread 3 (CPU#2 [running]) 0xf010027f in mp_main () at kern/init.c:130
  4    Thread 4 (CPU#3 [halted ]) 0x000fd412 in ?? ()
(gdb) 


4. mpentry.S se ejecuta en cada una de las CPUs. Se empieza a ejecutar en la dirección virtual mapeada a la dirección física 0x7000, que al estar ejecutándose en Real Mode resulta ser la misma dirección (0x7000)

La ejecución no se detiene al poner un breakpoint en mpentry_start porque el valor del símbolo en sí nada tiene que ver con el del instruction pointer.

5. nicolas@nicolas-PC:~/sisop$ make gdb ~

(gdb) b *0x7000 thread 4
Punto de interrupción 1 at 0x7000
(gdb) c
Continuando.

Thread 2 received signal SIGTRAP, Trace/breakpoint trap.
[Cambiando a Thread 2]
Se asume que la arquitectura objetivo es i8086
[ 700:   0]    0x7000:	cli    
0x00000000 in ?? ()

(gdb) p $eip
$1 = (void (*)()) 0x0

(gdb) disable 1

(gdb) si 10
Se asume que la arquitectura objetivo es i386
=> 0x7020:	mov    $0x10,%ax
0x00007020 in ?? ()

(gdb) x/10i $eip
=> 0x7020:	mov    $0x10,%ax
   0x7024:	mov    %eax,%ds
   0x7026:	mov    %eax,%es
   0x7028:	mov    %eax,%ss
   0x702a:	mov    $0x0,%ax
   0x702e:	mov    %eax,%fs
   0x7030:	mov    %eax,%gs
   0x7032:	mov    $0x11f000,%eax
   0x7037:	mov    %eax,%cr3
   0x703a:	mov    %cr4,%eax

(gdb) watch $eax == 0x11f000
Watchpoint 2: $eax == 0x11f000

(gdb) c
Continuando.
=> 0x7037:	mov    %eax,%cr3

Thread 2 hit Watchpoint 2: $eax == 0x11f000

Old value = 0
New value = 1
0x00007037 in ?? ()

(gdb) p $eip
$2 = (void (*)()) 0x7037

(gdb) p mpentry_kstack
$3 = (void *) 0x0

(gdb) si 8
=> 0x704e:	mov    0xf0237f04,%esp
0x0000704e in ?? ()

(gdb) p mpentry_kstack
$5 = (void *) 0xf0249000 <percpu_kstacks+65536>



ipc_recv:
--------------


Dado que queremos enviar el valor numerico -E_INVAL, r siempre va a ser negativo (ya sea porque dio un error o porque efectivamente devolvimos -E_INVAL).
Entonces, en la version A, tenemos que fijarnos el valor src ya que esta variable valdra 0 en caso de que ipc_recv haya fallado.

Version A:
  if( src == 0)
    puts("HUbo error.")

En la version B esto no es posible ya que como primer parametro de la funcion ipc_recv le pasamos NULL por lo que no podemos obtener esta informacion extra.
