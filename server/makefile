SHLIBDIR=-L${HOME}/lib
BINDIR=.
CC=g++
INCLUDE=-I./
VPATH=./

#LINKLIB=

all:server

server:server.cpp
	${CC}  -g ./server.cpp -o  server  ${DEBUG} ${CCFLAG} ${LDFLAG} ${LIBINCL} ${LINKLIB}
clean:
	rm ${BINDIR}/server

