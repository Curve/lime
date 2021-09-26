#!/bin/bash
colors=('\033[0;32m' '\033[0;33m' '\033[0;31m')

function print 
{
    echo -e "$1" "${@:2}" '\033[0m'
}
function is_number
{
    if [[ $1 = ?(-)+([0-9]) ]]; then
        return 0
    fi
    
    return 1
}

param_num="$#"
if [ $param_num -lt 2 ]; then
    print "${colors[2]}" "Too few arguments provided\nUsage: load <process_name> <library>\n"
    exit 1
fi

unload=false
for arg; do
    if [ "$arg" == "--unload" ]; then
        unload=true
        break
    fi
done

if [ $unload = true ]; then
    print "${colors[1]}" "Unloading specified library"
fi

process="$1"
library="$2"

pid=$(pidof "$process")
if [ -z "$pid" ] || [ -z "$library" ]; then
    if ! is_number "$process"; then
        print "${colors[2]}" "Could not find process with name $process"
        exit 1
    else
        pid=$process
        if ! [ -e "/proc/$pid" ]; then
            print "${colors[2]}" "Could not find process with pid $process"
            exit 1
        fi
    fi
fi

if [ $unload = true ]; then
    if grep -q "$library" "/proc/$pid/maps"; then
            print "${colors[0]}" "Library found"
    fi
else
    if grep -q "$library" "/proc/$pid/maps"; then
        print "${colors[1]}" "Already injected"
        exit 1
    fi
fi

echo "2" | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null

absolute_lib_path=$(realpath "$library")

if [ $unload = false ]; then
    gdb_result=$(sudo gdb -n -q -batch-silent \
                    -ex "set logging on" \
                    -ex "set logging file /dev/null" \
                    -ex "set logging redirect on" \
                    -ex "attach $pid" \
                    -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
                    -ex "call \$dlopen(\"$absolute_lib_path\", 1)" \
                    -ex "detach" \
                    -ex "quit"
                )
else
    gdb_result=$(sudo gdb -n -q -batch-silent \
                    -ex "set logging on" \
                    -ex "set logging file /dev/null" \
                    -ex "set logging redirect on" \
                    -ex "attach $pid" \
                    -ex "set \$dlopen = (void*(*)(char*, int)) dlopen" \
                    -ex "set \$dlclose = (int(*)(void*)) dlclose" \
                    -ex "set \$library = \$dlopen(\"$absolute_lib_path\", 6)" \
                    -ex "call \$dlclose(\$library)" \
                    -ex "call \$dlclose(\$library)" \
                    -ex "detach" \
                    -ex "quit"
                )
fi

if [ -n "$gdb_result" ]; then
    print "${colors[2]}" "Something went wrong: $gdb_result"
else
    print "${colors[0]}" "Done! ($gdb_result)"
fi

echo "0" | sudo tee /proc/sys/kernel/yama/ptrace_scope > /dev/null