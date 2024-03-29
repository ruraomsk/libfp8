# для других проектов изменить конечный каталог
NAMEPROJ = fp8
MAKEPATH = $(shell pwd)
DIRPATH = /root/proj
DDPATH = /root/proj/$(NAMEPROJ)
DRPATH = drivers
OBJDIR = obj
ifeq "$(AKEY)" "D"
DK = -g
endif
ifeq "$(AKEY)" "R"
DK = -O2
endif
ifeq "$(SYS)" "G"
INCLPATH = /usr/local/include/$(NAMEPROJ)
LIBPATH = /usr/local/lib
endif
ifeq "$(SYS)" "OW"
INCLPATH = /usr/include/$(NAMEPROJ)
LIBPATH = /usr/lib
endif
MKEY = -c $(DK) -DLINUXMODE -fPIC -MMD -MP -MF
ALLFILES = $(wildcard *.c)
OBJECTS := $(patsubst %.c, %.o, $(wildcard *.c))
OBJDRIVE := $(patsubst %.c, %.o, $(wildcard $(DRPATH)/*.c))
ALLOBJ = $(OBJECTS) $(OBJDRIVE)
IP = NULL

ifeq "$(SYS)" "G"
all: clean mObj mLib install
endif
ifeq "$(SYS)" "OW"
all: clean mObj MOpWrt
endif

mObj: cdir $(ALLOBJ)
$(ALLOBJ):
	gcc $(MKEY) "$(MAKEPATH)/$(OBJDIR)/$@.d" -o $(MAKEPATH)/$(OBJDIR)/$@ $(patsubst %.o, %.c, $@)

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

MOpWrt:  
	gcc -L/usr/lib -lmodbus -o $(LIBPATH)/lib$(NAMEPROJ).so $(wildcard $(MAKEPATH)/$(OBJDIR)/*.o) $(wildcard $(MAKEPATH)/$(OBJDIR)/$(DRPATH)/*.o) -shared -fPIC
	@mkdir -p $(INCLPATH)/$(DRPATH)
	@cp $(MAKEPATH)/*.h* $(INCLPATH)
	@cp $(MAKEPATH)/$(DRPATH)/*.h* $(INCLPATH)/$(DRPATH)

cdir:
	@mkdir -p $(MAKEPATH)/$(OBJDIR)/$(DRPATH)

clean:
	@rm -f $(MAKEPATH)/$(OBJDIR)/*.o*
	@rm -f $(MAKEPATH)/$(OBJDIR)/$(DRPATH)/*.o*
# принимает параметр IP=%
copyTo:
	ssh root@$(IP) 'mkdir -p $(DDPATH)/$(DRPATH)'
	-scp $(MAKEPATH)/* root@$(IP):$(DDPATH)
	-scp $(MAKEPATH)/$(DRPATH)/* root@$(IP):$(DDPATH)/$(DRPATH)
	ssh root@$(IP) 'make -f $(DDPATH)/Makefile DevInst'