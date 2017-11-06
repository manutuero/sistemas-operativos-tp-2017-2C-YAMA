clean:
	 rm dataNode
all: dataNode

dataNode:
	@echo 'Building dataNode'
	gcc src/*.c -o dataNode -lcommons -lutils


install:
	mkdir -p /home/utnso/thePonchos/
	touch /home/utnso/thePonchos/data.bin
	truncate -s 314572800 /home/utnso/thePonchos/data.bin
