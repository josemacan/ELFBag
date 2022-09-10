# PLUGIN CLANG 11

## Instalacion de CLANG 11

0. Chequear la version base de CLANG/LLVM del sistema
``` Bash
$ clang -v

FreeBSD clang version 11.0.1 (git@github.com:llvm/llvm-project.git llvmorg-11.0.1-0-g43ff75f2c3fe)
Target: x86_64-unknown-freebsd13.0
Thread model: posix
InstalledDir: /usr/bin

```


1. Instalar la version 11 de CLANG/LLVM

``` Bash
$ sudo pkg install llvm11
```

2. Chequear que se haya instalado efectivamente la version 11 desde pkg
    En este momento se van a tener dos partes: 1) LLVM 11 base del sistema (en /usr/bin/clang). 2) LLVM 11 desde pkg (en /usr/local/llvm11)

``` Bash
$ clang11 -v

clang version 11.0.1
Target: x86_64-portbld-freebsd13.0
Thread model: posix
InstalledDir: /usr/local/llvm11/bin
```

## Compilacion del plugin de CLANG

1. Dirigirse al directorio donde esta el codigo fuente del plugin
``` Bash
$ cd CLANGPlugin
```

2. Encontrar el directorio de instalación de clang11
    ``` Bash
    $ clang11 -v

    clang version 11.0.1
    Target: x86_64-portbld-freebsd13.0
    Thread model: posix
    InstalledDir: /usr/local/llvm11/bin
    ```

3. Dentro del directorio dado por InstalledDir del paso anterior, buscar donde se encuentra
    el archivo de CMake: **ClangConfig.cmake**.
    ``` Bash
    $ find / -name ClangConfig.cmake
    ```
    En mi caso está en: **/usr/local/llvm11/lib/cmake/clang**

4. Setear las variables de entorno. (Lo hace automáticamente el script de paso 5)

- Clang_DIR: path al directorio donde se encuentra *ClangConfig.cmake* (ver 3)
- CLANG_TUTOR_DIR: path al directorio de codigo fuente del plugin 

5. Ejecutar el script **setup.sh** para iniciar CMake y compilar el plugin

    ``` Bash
    $ cd CLANGPlugin
    $ sh setup.sh

    # setup.sh

    DIR=$(pwd)

    export CLANG_TUTOR_DIR=$DIR
    export Clang_DIR=/usr/local/llvm11

    cd $DIR
    mkdir build
    cd $DIR/build
    cmake -DCT_Clang_INSTALL_DIR=$Clang_DIR $CLANG_TUTOR_DIR

    cd $DIR/build
    make

    # setup.sh output

    -- The C compiler identification is Clang 11.0.1
    -- The CXX compiler identification is Clang 11.0.1
    -- Detecting C compiler ABI info
    -- Detecting C compiler ABI info - done
    -- Check for working C compiler: /usr/bin/cc - skipped
    -- Detecting C compile features
    -- Detecting C compile features - done
    -- Detecting CXX compiler ABI info
    -- Detecting CXX compiler ABI info - done
    -- Check for working CXX compiler: /usr/bin/c++ - skipped
    -- Detecting CXX compile features
    -- Detecting CXX compile features - done
    -- Configuring done
    -- Generating done
    -- Build files have been written to: /home/freebsd/CLANGPlugin/build
    /usr/local/bin/cmake -S/home/freebsd/CLANGPlugin -B/home/freebsd/CLANGPlugin/build --check-build-system CMakeFiles/Makefile.cmake 0
    /usr/local/bin/cmake -E cmake_progress_start /home/freebsd/CLANGPlugin/build/CMakeFiles /home/freebsd/CLANGPlugin/build//CMakeFiles/progress.marks
    make  -f CMakeFiles/Makefile2 all
    make  -f CMakeFiles/InsertPayload_Plugin.dir/build.make CMakeFiles/InsertPayload_Plugin.dir/depend
    cd /home/freebsd/CLANGPlugin/build && /usr/local/bin/cmake -E cmake_depends "Unix Makefiles" /home/freebsd/CLANGPlugin /home/freebsd/CLANGPlugin /home/freebsd/CLANGPlugin/build /home/freebsd/CLANGPlugin/build /home/freebsd/CLANGPlugin/build/CMakeFiles/InsertPayload_Plugin.dir/DependInfo.cmake --color=
    make  -f CMakeFiles/InsertPayload_Plugin.dir/build.make CMakeFiles/InsertPayload_Plugin.dir/build
    [ 50%] Building CXX object CMakeFiles/InsertPayload_Plugin.dir/insertPayload_Plugin.cpp.o
    /usr/bin/c++ -DInsertPayload_Plugin_EXPORTS -I/usr/src/sys -isystem /usr/local/llvm11/include -fPIC -MD -MT CMakeFiles/InsertPayload_Plugin.dir/insertPayload_Plugin.cpp.o -MF CMakeFiles/InsertPayload_Plugin.dir/insertPayload_Plugin.cpp.o.d -o CMakeFiles/InsertPayload_Plugin.dir/insertPayload_Plugin.cpp.o -c /home/freebsd/CLANGPlugin/insertPayload_Plugin.cpp
    [100%] Linking CXX shared library libInsertPayload_Plugin.so
    /usr/local/bin/cmake -E cmake_link_script CMakeFiles/InsertPayload_Plugin.dir/link.txt --verbose=1
    /usr/bin/c++ -fPIC -shared -Wl,-soname,libInsertPayload_Plugin.so -o libInsertPayload_Plugin.so CMakeFiles/InsertPayload_Plugin.dir/insertPayload_Plugin.cpp.o
    [100%] Built target InsertPayload_Plugin
    /usr/local/bin/cmake -E cmake_progress_start /home/freebsd/CLANGPlugin/build/CMakeFiles 0
    ```

6. Compilar el codigo fuente *test.c* del directorio /test/src usando el plugin obtenido ejecutando el script **test.sh**,       pasando como 
    primer argumento el nombre del código fuente del programa de usuario. El resto de los argumentos son el nombre 
    de los archivos que tienen datos a insertar en el ejecutable final. Modificar los flags de compilación necesarios dentro del script.

    ``` Bash
    $ sh test.sh test/src/test.c test/dataToInsert/decl_payA.txt test/dataToInsert/decl_payB.txt

    # test.sh

    User program source code: test/src/test.c
    
    Plugin argument - 2: test/dataToInsert/decl_payA.txt
    Plugin argument - 3: test/dataToInsert/decl_payB.txt
    
    Count of arguments: 3
    
    clang11 -Xclang -load -Xclang ./build/libInsertPayload_Plugin.so  -Xclang -plugin -Xclang InsertPayload_Plugin   -Xclang -plugin-arg-InsertPayload_Plugin -Xclang test/dataToInsert/decl_payA.txt -Xclang -plugin-arg-InsertPayload_Plugin -Xclang test/dataToInsert/decl_payB.txt  -c test/src/test.c -I/usr/src/sys -Isrc  -Wno-implicit-int
    
    Args[0] = test/dataToInsert/decl_payA.txt
    Args[1] = test/dataToInsert/decl_payB.txt
    
    Output file created - /tmp/metadata_test.c
    
    clang11 -o metadataTestProgram /tmp/metadata_test.c -Isrc  -I/usr/src/sys
    
    rm /tmp/metadata_test.c
    ```

8. El archivo modificado se encuentra en el path /tmp con el prefijo **metadata_**

    ``` Bash
    $ cd /tmp
    $ cat metadata_test.c

    #include <stdio.h>
    #include <string.h>
    #include <stddef.h>
    #include <inttypes.h>
    #include <sys/metadata_payloads.h>

    static Metadata_Hdr metadata_header __attribute__((__used__, __section__(".metadata"))) = {2, sizeof(Payload_Hdr)};static Payload_Binary payloads[] __attribute__((__used__, __section__(".metadata"), __aligned__(8))) = {{24,48,"5061796C6F61645F412F322F342F632F696E745F73697A65"},{26,52,"5061796C6F61645F422F31362F7A2F772F636861725F73697A65"}};static Payload_Hdr payload_headers[] __attribute__((__used__, __section__(".metadata"))) = {{3, sizeof(payloads[0])},{3, sizeof(payloads[1])}};

    int main(int argc, char *argv[]) {

            printf("\n******** codeinsertion finished ********\n");

            return 0;
    }
    ```

9. El programa compilado (formato ejecutable ELF) se almacena en el directorio principal del desarrollo (*...\CLANGPlugin*) bajo el nombre **metadataTestProgram**


## REFERENCIAS

[Frequently Asked Questions (FAQ) - Clang 15.0 documentation - Driver](https://clang.llvm.org/docs/FAQ.html)

[Clang 11 documentation - Index](https://releases.llvm.org/11.0.0/tools/clang/docs/genindex.html)

[Clang 11 documentation - Clang Plugins - Running the plugin](https://releases.llvm.org/11.0.0/tools/clang/docs/ClangPlugins.html#running-the-plugin)

[StackOverFlow - "clang -cc1 and system includes"](https://stackoverflow.com/questions/18506590/clang-cc1-and-system-includes)