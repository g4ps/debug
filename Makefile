CFLAGS = -g 
obj = dump.o main.o parse_input.o
out = dump 

all: ${out}

.PHONY: all clean

dump: ${obj}
	cc $^ ${CFLAGS} -o $@

clean:
	-rm ${out} ${obj}
