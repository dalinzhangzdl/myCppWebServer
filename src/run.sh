mkdir ../log
logfile=$(date "+%Y-%m-%d-%H:%M:%S")
./server 8080 4 | tee "../log/"${logfile}".log"
