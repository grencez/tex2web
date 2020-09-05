
BldPath=bld
BinPath=bin

SrcPath=src
DepPath=dep
CxTopPath=$(DepPath)/cx

CMAKE=cmake
GODO=$(CMAKE) -E chdir
MKDIR=$(CMAKE) -E make_directory
CTAGS=ctags

.PHONY: default all cmake proj \
	test tags clean distclean \
	init update pull

default:
	$(MAKE) init
	if [ ! -d $(BldPath) ] ; then $(MAKE) cmake ; fi
	$(MAKE) proj

all:
	$(MAKE) init
	$(MAKE) cmake
	$(MAKE) proj

cmake:
	if [ ! -d $(BldPath) ] ; then $(MKDIR) $(BldPath) ; fi
	$(GODO) $(BldPath) $(CMAKE) ../src

proj:
	$(GODO) $(BldPath) $(MAKE)

test:
	$(GODO) $(BldPath) $(MAKE) test

tags:
	$(CTAGS) -R src -R dep/cx/src

clean:
	$(GODO) $(BldPath) $(MAKE) clean

distclean:
	$(GODO) $(CxTopPath) $(MAKE) distclean
	rm -fr $(BldPath) $(BinPath)

init:
	if [ ! -f $(DepPath)/cx/src/cx.c ] ; then git submodule init dep/cx ; git submodule update dep/cx ; fi
	if [ ! -f $(DepPath)/cx-pp/cx.c ] ; then git submodule init dep/cx-pp ; git submodule update dep/cx-pp ; fi

update:
	git pull origin master
	git submodule update
	git submodule foreach git checkout master
	git submodule foreach git merge --ff-only origin/master

pull:
	git pull origin master
	git submodule foreach git pull origin master

