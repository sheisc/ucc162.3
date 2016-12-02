UCCDIR = /home/iron/bin
export UCCDIR

all:
	make -C driver
	make -C ucl

clean:
	make -C driver clean
	make -C ucl clean
	
install:
	mkdir -p $(UCCDIR)
	cp driver/ucc $(UCCDIR)
	cp ucl/ucl $(UCCDIR)
	cp ucl/assert.o $(UCCDIR)
	cp -r ucl/linux/include $(UCCDIR)
uninstall:
	cd $(UCCDIR);rm -f ucl assert.o ucc ucl.bak ucl1 ucl2;rm -rf $(UCCDIR)/include
test:
	make -C ucl test

