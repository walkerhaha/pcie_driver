#!/bin/bash


function rand_16() {
	min=$1
	max=$(($2-$min+1))
	num=$(($RANDOM+10000000000000000))
	echo $(($num%$max+$min))
}

function rand_32() {
	low=$(rand_16 0 32767)
	high=$(rand_16 0 32767)
	echo $(($high*32768+low))
}

function rand_req() {
	for((i=1;i<=$2;i++));
	do
		if [ $(($i%1000)) -eq 0 ]
                then
                                printf "test_cnt=%d\n" $i
                fi
		addr=$(($(rand_32)+0x58000000000+0x400000000))
		if [ "$1" -eq 1 ]
		then
			#printf "0x%x\n" $addr
			busybox devmem $addr 32
			if [ $i -eq $2 ]
			then
				printf "test done\n"
			fi
		elif [ "$1" -eq 0 ]
		then
			data=$(rand_32)
			#printf "0x%x 0x%x\n" $addr $data
			busybox devmem $addr 32 $data
			if [ $i -eq $2 ]
                        then
                                printf "test done\n"
                        fi
		else
			printf "parameter error\n"
		fi
	done
}

function cont_req() {
        for((i=1;i<=$2;i++));
        do
		addr=$(($i + 0x58400000000))
                if [ "$1" -eq 1 ]
                then
                        #printf "0x%x\n" $addr
                        busybox devmem $addr 32
                        if [ $i -eq $2 ]
                        then
                                printf "test done\n"
                        fi
                elif [ "$1" -eq 0 ]
                then
                        data=$(rand_32)
                        #printf "0x%x 0x%x\n" $addr $data
                        busybox devmem $addr 32 $data
                        if [ $i -eq $2 ]
                        then
                                printf "test done\n"
                        fi
                else
                        printf "parameter error\n"
                fi
        done
}


rm -rf rand_r.log
rm -rf rand_w.log
rand_req 1 2000 >> rand_r.log
rand_req 0 10000 >> rand_w.log
rand_req 1 2000 >> rand_r.log
rand_req 0 10000 >> rand_w.log
rand_req 1 2000 >> rand_r.log
rand_req 0 10000 >> rand_w.log
