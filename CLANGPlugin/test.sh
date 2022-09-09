#!/bin/sh

# The Clang driver accepts the -fplugin option to load a plugin. Clang plugins can receive arguments 
# from the compiler driver command line via the fplugin-arg-<plugin name>-<argument> option. 
# Using this method, the plugin name cannot contain dashes itself, but the argument passed to the plugin can.

# $1 = user program source code name 
# rest of arguments = plugin arguments containing data to insert in user program's executable

# EXAMPLE
# $ sh test.sh test.c decl_payA.txt_binary.bin decl_payB.txt_binary.bin

arguments="";
#export args_passed="@";
args_count=$#;

#clang_command="";

compiler="clang11"
plugin_so_name="libInsertPayload_Plugin.so"
plugin_so_dir="./build"
plugin_name="InsertPayload_Plugin"
flag_clang_usage="-Xclang"
flag_arg_clang="-plugin-arg-"
compiler_flags="-Wno-implicit-int" # Flags needed to compile the plugin and the modified user source code
src_dir="src"
user_compiler_flags="-I$src_dir "    # User should add here the different flags for their program to compile
user_program_name="metadataTestProgram"
tmp_dir="/tmp/"
modified_src_code_prefix="metadata_"

include_sys_dir="/usr/src/sys"

if [ -z "$1" ]; then
    echo "ERROR: User program source code has not been defined as the first argument."
    exit 1
fi

src_code="$1"
src_code_filename="$src_code"

if [ -n "$src_dir" ]; then
    src_code="$src_dir/$1"
fi

echo "User program source code: $1"

i=2;
while [ "$i" -le "$args_count" ]
do
    arguments="$arguments $flag_clang_usage $flag_arg_clang$plugin_name $flag_clang_usage $2";
    echo "Plugin argument - $i: $2";
    i=$((i + 1));
    shift 1;
done

echo "Count of arguments: $args_count"
#echo "$arguments"

## COMPILE AND PLUGIN USAGE COMMAND

clang_command="$clang_command $compiler $flag_clang_usage -load $flag_clang_usage $plugin_so_dir/$plugin_so_name "
clang_command="$clang_command $flag_clang_usage -plugin $flag_clang_usage $plugin_name "
clang_command="$clang_command $arguments "
clang_command="$clang_command -c $src_code -I$include_sys_dir $user_compiler_flags $compiler_flags"

echo "$clang_command"

eval "$clang_command"

# clang11 -Xclang -load  -Xclang ./build/libRenameMacro_Payload.so -Xclang -plugin -Xclang InsertPayload_Plugin -Xclang -plugin-arg-InsertPayload_Plugin -Xclang decl_payA.txt_binary.bin -Xclang -plugin-arg-InsertPayload_Plugin -Xclang decl_payB.txt_binary.bin  -c src/test.c -Isrc -I/usr/src/sys

## COMPILE THE PROGRAM
compile_command="$compiler -o $user_program_name $tmp_dir$modified_src_code_prefix$src_code_filename $user_compiler_flags -I$include_sys_dir"

echo "$compile_command"

eval "$compile_command"

## REMOVE THE TMP FILE

remove_tmp_src_command="rm $tmp_dir$modified_src_code_prefix$src_code_filename"

echo "$remove_tmp_src_command"

eval "$remove_tmp_src_command"