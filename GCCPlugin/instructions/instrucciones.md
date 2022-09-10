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
$ cd GCCPlugin
$ sh setup.sh

g++11 -shared -O2 -pipe -fPIC -fno-rtti -O2 -I/usr/src/sys -I/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/11.2.0/plugin/include insertPayload_GCCPlugin.c -o InsertPayload_GCCPlugin.so 
```

5. Compilar programa cargando el plugin de GCC. Ejecutar el script **test.sh**  
   El primer argumento es el nombre del código fuente del programa de usuario (**prueba.c** del directorio /test/src). El resto de los argumentos son el nombre de los archivos que tienen datos a insertar en el ejecutable final.

``` Bash
$ cd GCCPlugin
$ sh test.sh test/src/test.c test/dataToInsert/decl_payA.txt test/dataToInsert/decl_payB.txt

User program source code: test/src/test.c

Plugin argument - 2: test/dataToInsert/decl_payA.txt
Plugin argument - 3: test/dataToInsert/decl_payB.txt

Count of arguments: 3

make test PLUGIN_ARGS='-fplugin-arg-InsertPayload_GCCPlugin-1=test/dataToInsert/decl_payA.txt -fplugin-arg-InsertPayload_GCCPlugin-1=test/dataToInsert/decl_payB.txt '

gcc11 -o metadataTestProgramGCC test/src/prueba.c -fplugin=./InsertPayload_GCCPlugin.so -fplugin-arg-InsertPayload_GCCPlugin-1=test/dataToInsert/decl_payA.txt -fplugin-arg-InsertPayload_GCCPlugin-1=test/dataToInsert/decl_payB.txt  -I/usr/src/sys
```

6. Ejecutar programa con metadata insertada

``` Bash
$ cd GCCPlugin
$ ./metadataTestProgramGCC
```

7. Chequear que metadata fue extraida del ejecutable "metadataTestProgramGCC" en el log de kernel

``` Bash
$ dmesg
```