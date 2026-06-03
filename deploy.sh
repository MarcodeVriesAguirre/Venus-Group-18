#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "usage: $0 <javier|julius|both>" >&2
    exit 1
}

deploy_one() {
    local host=$1
    echo "[$host] syncing..."
    rsync -az --delete \
        --exclude '*.o' --exclude '*.d' --exclude 'main' \
        Pynq/robot/ "$host:~/Venus-Group-18/Pynq/robot/"
    echo "[$host] building and running..."
    ssh -t "$host" 'cd ~/libpynq-5EID0-2023-v0.3.0/applications/robot && make && sudo ./main' \
        2>&1 | sed "s/^/[$host] /"
}

[[ $# -ge 1 ]] || usage

case "$1" in
    javier|julius)
        deploy_one "$1"
        ;;
    both)
        deploy_one javier &
        deploy_one julius &
        wait
        ;;
    *)
        usage
        ;;
esac
