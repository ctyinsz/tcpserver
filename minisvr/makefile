SHLIBDIR=-L${HOME}/lib
BINDIR=.
CC=gcc
INCLUDE=-I./
VPATH=./

#LINKLIB=

all:minisvr mtminisvr mt2minisvr

minisvr:minisvr.c
	${CC}  -g ./minisvr.c -o  minisvr  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
mtminisvr:minisvr_mt.c
	${CC}  -g ./minisvr_mt.c -o  mtminisvr  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
mt2minisvr:minisvr_mt2.c
	${CC}  -g ./minisvr_mt2.c -o  mt2minisvr  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
clean:
	rm ${BINDIR}/minisvr
	rm ${BINDIR}/mtminisvr
	rm ${BINDIR}/mt2minisvr

