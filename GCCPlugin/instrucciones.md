# INSTALACION GCC 11 en FREEBSD  - RAW BINARY METADATA

1. Instalar pkgs de GCC 11
``` Bash
$ sudo pkg install gcc11
```

2. Chequear que el header "gcc-plugin.h" esté en el path

/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/11.2.0/plugin/include

3. Modificar el Makefile para que coincida con la subversion de GCC que se ha instalado: 

/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/**XXXXX**/plugin/include

4. Compilar plugin

``` Bash
$ cd PI/MetadataGCCPlugin/PluginGCCFreeBSD
$ sh setup.sh

g++11 -shared -O2 -pipe -I/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/11.2.0/plugin/include -fPIC -fno-rtti -O2 -I/usr/src/sys insertPayload_GCCPlugin.c -o InsertPayload_GCCPlugin.so
```

5. Compilar programa cargando el plugin de GCC. Ejecutar el script **test.sh**. 
   El primer argumento es el nombre del código fuente del programa de usuario. El resto de los argumentos son el nombre de los archivos que tienen datos a insertar en el ejecutable final.

``` Bash
$ cd PI/MetadataGCCPlugin/PluginGCCFreeBSD
$ sh test.sh prueba.c decl_payA.txt_binary.bin decl_payB.txt_binary.bin

gcc11 -o insertedProg prueba.c -fplugin=./InsertPayload_GCCPlugin.so -fplugin-arg-InsertPayload_GCCPlugin-3=decl_payA.txt_binary.bin -I/usr/src/sys
```

6. Ejecutar programa con metadata insertada

``` Bash
$ cd PI/MetadataGCCPlugin/PluginGCCFreeBSD
$ ./insertedProg
```

7. Chequear que metadata fue extraida del ejecutable "insertedProg" en el log de kernel

``` Bash
$ dmesg
```