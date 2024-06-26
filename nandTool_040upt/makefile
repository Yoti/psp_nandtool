export RELVER := 0.4UPT
RELFN := _040UPT

all: elf eboot

release: clean
	@rm -r -f nandTool$(RELFN)
	mkdir -p nandTool$(RELFN)/PSP/GAME/nandTool$(RELFN)
	mkdir nandTool$(RELFN)/elf/
	@tar -cvjf nandTool_$(RELVER)_src.tar.bz2 source makefile readme.txt changelog.txt todo.txt
	@mv nandTool_$(RELVER)_src.tar.bz2 nandTool$(RELFN)
	make all
	cp -f nandTool$(RELFN).elf nandTool$(RELFN)/elf/nandTool$(RELFN).elf
	cp EBOOT.PBP nandTool$(RELFN)/PSP/GAME/nandTool$(RELFN)/EBOOT.PBP
	cp -f ntbridge.prx nandTool$(RELFN)/PSP/GAME/nandTool$(RELFN)/ntbridge.prx
	cp -f ntbridge.prx nandTool$(RELFN)/elf/ntbridge.prx
	cp readme.txt nandTool$(RELFN)/readme.txt
	cp changelog.txt nandTool$(RELFN)/changelog.txt
	cp todo.txt nandTool$(RELFN)/todo.txt

clean:
	@rm -f nandTool$(RELFN).elf
	@rm -f ntbridge.prx
	@rm -f EBOOT.PBP
	@rm -r -f nandTool$(RELFN)
	make -C source clean
	make -C source -f ebt_makefile clean
	make -C source/graphics clean
	make -C source/ntbridge clean

elf: libs
	make -C source
	@cp -f source/nandTool_strip.elf nandTool$(RELFN).elf
	make -C source clean

eboot: libs
	make -C source -f ebt_makefile
	@cp -f source/EBOOT.PBP EBOOT.PBP
	make -C source -f ebt_makefile clean

libs:
	make -C source/graphics
	make -C source/ntbridge
	@cp -f source/ntbridge/ntbridge.prx ntbridge.prx
	make -C source/ntbridge clean
