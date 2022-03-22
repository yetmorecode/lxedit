lxedit.exe: lxedit.obj objects.obj pages.obj fixups.obj info.obj
	wlink @<< 
		name $@ 	
		system dos32a
		file {
			..\..\pdcurses\dos\pdcurses.lib 
			$<
		}
		option quiet
<<

lxedit.obj: lxedit.c lx.h
	wpp386 -zq -bt=dos32a $< -i=..\..\pdcurses
objects.obj: objects.c lx.h
	wpp386 -zq -bt=dos32a $< -i=..\..\pdcurses
pages.obj: pages.c lx.h
	wpp386 -zq -bt=dos32a $< -i=..\..\pdcurses
fixups.obj: fixups.c lx.h
	wpp386 -zq -bt=dos32a $< -i=..\..\pdcurses
info.obj: info.c lx.h
	wpp386 -zq -bt=dos32a $< -i=..\..\pdcurses

clean: .SYMBOLIC
	del *.obj
	del lxedit.exe
