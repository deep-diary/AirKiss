.PHONY:clean
OBJECTS= promisc.o libairkiss_log.a
promisc_main:$(OBJECTS)
	mips-openwrt-linux-gcc -Wall -g $^ -o $@ 
promisc.o:promisc.c
	mips-openwrt-linux-gcc -Wall -g -c $< -o $@

clean:
	rm -f $(OBJECTS) promisc_main
