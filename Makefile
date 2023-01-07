
CC = g++

CEXE = ./client
SEXE = ./server
CSRC_DIR = ./src/client
SSRC_DIR = ./src/server
CORE_SRC_DIR = ./src/core

ctargs = $(wildcard $(CSRC_DIR)/*.cpp)
stargs = $(wildcard $(SSRC_DIR)/*.cpp)
core_targs = $(wildcard $(CORE_SRC_DIR)/*.cpp)

args = -Wall -pthread -Wno-unused-variable -Wno-sign-compare

all: client server

client:
	${CC} -o ${CEXE} ${ctargs} ${core_targs} ${args}

server:
	${CC} -o ${SEXE} ${stargs} ${core_targs} ${args}

clean: cleanc cleans

cleanc:
	rm ${CEXE}
cleans:
	rm ${SEXE}