SHLIBDIR=-L${HOME}/lib
BINDIR=.
CC=g++
CC2=gcc
INCLUDE=-I./
VPATH=./

LINKLIB=-pthread

all:server server_mt workflow

server:server.cpp
	${CC}  -g ./server.cpp -o  server  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
server_mt:server_mt.c
	${CC2}  -g ./server_mt.c -o  mtserver  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
workflow:workflow.c
	${CC2}  -g ./workflow.c -o  workflow  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
clean:
	rm ${BINDIR}/server
	rm ${BINDIR}/mtserver
	rm ${BINDIR}/workflow

