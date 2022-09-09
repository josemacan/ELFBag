#!/bin/sh

# Script to compile the GCC plugin

fbsd_version=$(freebsd-version)
#echo "FreeBSD version: $fbsd_version"
version_num=""
dir_gcc_plugin_header=""

old_IFS="$IFS"

IFS='-'

for word in $fbsd_version 
do 
    version_num=$word; 
    break; 
done

IFS="$old_IFS"

#echo "version num: $version_num" 

case $version_num in
        "12.3")
                dir_gcc_plugin_header="/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd12.2/11.2.0/plugin/include"
                ;;
        "13.0")
                dir_gcc_plugin_header="/usr/local/lib/gcc11/gcc/x86_64-portbld-freebsd13.0/11.2.0/plugin/include"
                ;;
        "11.0")
                echo "FreeBSD Version $version_num does not support GCC 11"
                exit 1
                ;;
        *)
                echo "FreeBSD Version $version_num NOT SUPPORTED yet"
                exit 1
                ;;
esac

#echo "DIR_GCC_PLUGIN_HEADER: $dir_gcc_plugin_header" 

make_cmd="make DIR_GCC_PLUGIN_HEADER='$dir_gcc_plugin_header'";

echo "$make_cmd"

eval "$make_cmd"