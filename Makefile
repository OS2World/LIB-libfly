all:
	$(MAKE) -C src $*

install:
	$(MAKE) -C src $* install

DIST_NAME=libfly
DIST_VERSION=1.0
	
dist: clean
	cd .. && tar cvf $(DIST_NAME)-$(DIST_VERSION).tar $(DIST_NAME)
	gzip ../$(DIST_NAME)-$(DIST_VERSION).tar
	
%:
	$(MAKE) -C src $*
	$(MAKE) -C samples $*
