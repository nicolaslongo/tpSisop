TP1: Memoria virtual en JOS
===========================

page2pa
-------

La función recibe un puntero a un struct PageInfo. En Jos, se utiliza ésta dirección de memoria en conjunto con la del comienzo del arreglo pages para deducir la dirección física.
En primer lugar, es necesario mencionar que el arreglo pages posee un puntero al comienzo de la lista de páginas. Si a una página cualquiera le restamos el comienzo del arreglo, obtenemos la dirección relativa.
En segundo lugar, este arreglo está almacenado en memoria física cuyo mapeo en memoria virtual es correlativo (1 a 1). Esto quiere decir que si restamos una página determinada con el inicio de la lista obtenemos la posición en memoria física de dicha página.


boot_alloc_pos
--------------

a)
$ nm kernel
	f0115970 B end
Por lo tanto, la primera dirección que boot_alloc devolverá será 0xf0116000


b)

(gdb) b boot_alloc
Punto de interrupción 1 at 0xf01009a6: file kern/pmap.c, line 89.
(gdb) c
Continuando.
Se asume que la arquitectura objetivo es i386

Breakpoint 1, boot_alloc (n=65684) at kern/pmap.c:89
89	{
=> 0xf01009af <boot_alloc+9>:	cmpl   $0x0,0xf0115538
98		if (!nextfree) {
(gdb) p nextfree
$1 = 0x0
=> 0xf01009fc <boot_alloc+86>:	mov    $0xf011696f,%eax
100			nextfree = ROUNDUP((char *) end, PGSIZE);
(gdb) p (char*) &end
$9 = 0xf0115970 "\022"
(gdb) p nextfree
$4 = 0xf0116000 ""


page_alloc
----------

La función page2pa recibe un puntero a un struct PageInfo y devuelve su dirección física. La función page2kva devuelve la dirección virtual mapeada (por encima del Kernel, usando la macro KADDR) a la dirección física que contiene a dicha página.