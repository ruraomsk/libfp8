# для других проектов изменить конечный каталог
NAMEPROJ = fp8
MAKEPATH = $(shell pwd)
DIRPATH = /root/proj
DDPATH = /root/proj/$(NAMEPROJ)
DRPATH = drivers
OBJDIR = obj
AKEY = NULL
DMKEY = -c -g -DLINUXMODE -fPIC -MMD -MP -MF
RMKEY = -c -O2 -DLINUXMODE -fPIC -MMD -MP -MF
ALLFILES = $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(wildcard *.c))
OBJDRIVE := $(patsubst %.c, %.o, $(wildcard $(DRPATH)/*.c))
ALLOBJ = $(OBJECTS) $(OBJDRIVE)
INCLPATH = /usr/local/include/$(NAMEPROJ)
LIBPATH = /usr/local/lib
INCLPATHOPWRT = /usr/include/$(NAMEPROJ)
LIBPATHOPWRT = /usr/lib
IP = NULL

all: clean mObj mLib install

mObj: cdir $(ALLOBJ)

$(ALLOBJ):
ifeq "$(AKEY)" "D"
	gcc $(DMKEY) "$(MAKEPATH)/$(OBJDIR)/$@.d" -o $(MAKEPATH)/$(OBJDIR)/$@ $(patsubst %.o, %.c, $@)
endif
ifeq "$(AKEY)" "R"
	gcc $(RMKEY) "$(MAKEPATH)/$(OBJDIR)/$@.d" -o $(MAKEPATH)/$(OBJDIR)/$@ $(patsubst %.o, %.c, $@)
endif

mLib:
	gcc -o $(LIBPATH)/lib$(NAMEPROJ).so $(wildcard $(MAKEPATH)/$(OBJDIR)/*.o) $(wildcard $(MAKEPATH)/$(OBJDIR)/$(DRPATH)/*.o) -shared -fPIC

install:
	@mkdir -p $(INCLPATH)/$(DRPATH)
	@rm -f $(INCLPATH)/*.h*
	@rm -f $(INCLPATH)/$(DRPATH)/*.h*
	@cp $(MAKEPATH)/*.h* $(INCLPATH)
	@cp $(MAKEPATH)/$(DRPATH)/*.h* $(INCLPATH)/$(DRPATH)
	@cp $(MAKEPATH)/lib$(NAMEPROJ).pc /usr/lib/pkgconfig
	@cp $(MAKEPATH)/99$(NAMEPROJ)8 /etc/env.d
	@env-update && source /etc/profile
	@echo Lib installed.

DevInst:
	@mkdir -p $(INCLPATH)/$(DRPATH)
	@rm -f $(INCLPATH)/*.h*
	@rm -f $(INCLPATH)/$(DRPATH)/*.h*
	@cp $(DDPATH)/*.h* $(INCLPATH)
	@cp $(DDPATH)/$(DRPATH)/*.h* $(INCLPATH)/$(DRPATH)
	@cp $(DDPATH)/lib$(NAMEPROJ).pc /usr/lib/pkgconfig
	@cp $(DDPATH)/99$(NAMEPROJ)8 /etc/env.d
	@env-update && source /etc/profile
	@echo Lib installed.

MOpWrt: clean mObj 
	gcc -L/usr/lib -lmodbus -o $(LIBPATHOPWRT)/lib$(NAMEPROJ).so $(wildcard $(MAKEPATH)/$(OBJDIR)/*.o) $(wildcard $(MAKEPATH)/$(OBJDIR)/$(DRPATH)/*.o) -shared -fPIC
	@mkdir -p $(INCLPATHOPWRT)/$(DRPATH)
	@cp $(MAKEPATH)/*.h* $(INCLPATHOPWRT)
	@cp $(MAKEPATH)/$(DRPATH)/*.h* $(INCLPATHOPWRT)/$(DRPATH)

cdir:
	@mkdir -p $(MAKEPATH)/$(OBJDIR)/$(DRPATH)

clean:
	@rm -f $(MAKEPATH)/$(OBJDIR)/*.o*

# принимает параметр IP=%
copyTo:
	ssh root@$(IP) 'mkdir -p $(DDPATH)/$(DRPATH)'
	-scp $(MAKEPATH)/* root@$(IP):$(DDPATH)
	-scp $(MAKEPATH)/$(DRPATH)/* root@$(IP):$(DDPATH)/$(DRPATH)
	ssh root@$(IP) 'make -f $(DDPATH)/Makefile DevInst'