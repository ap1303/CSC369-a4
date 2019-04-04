all: cp mkdir ln rm

cp: ext2_cp.c ext2_helper.c
		gcc -Wall -g -o ext2_cp ext2_cp.c

mkdir: ext2_mkdir.c ext2_helper.c
		gcc -Wall -g -o ext2_mkdir ext2_mkdir.c

ln: ext2_ln.c ext2_helper.c
		gcc -Wall -g -o ext2_ln ext2_ln.c

rm: ext2_rm.c ext2_helper.c
		gcc -Wall -g -o ext2_rm ext2_rm.c

checker: ext2_checker.c ext2_helper.c
		gcc -Wall -g -o ext2_checker ext2_checker.c