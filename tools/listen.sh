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
output="/tmp/lime"
if [ $param_num -gt 0 ]; then
    output="/tmp/lime_$1"
fi

while [ ! -f "$output" ]; do
    print "\033c"
    print "${colors[2]}" "Waiting for output...\n"
    sleep 0.05
done

print "${colors[0]}" "Output found!"
tail -F "$output"