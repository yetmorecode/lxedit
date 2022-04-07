CURSES=..\pdcurses

lxedit.exe: lxlib.obj objects.obj pages.obj fixups.obj info.obj lxedit.obj
	wlink @<< 
		name $@ 	
		system dos32a
		file {
			$CURSES\dos\pdcurses.lib 
			$<
		}
		option quiet
<<

a.exe: a.obj
	wlink @<<
		name $@
		system dos32a
		file {
			$<
		}
		option quiet
<<
b.exe: b.obj
	wlink @<<
		name $@
		system stub32ac
		debug all
		file {
			$<
		}
<<
lxedit.obj: lxedit.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
lxlib.obj: lxlib.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
objects.obj: objects.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
pages.obj: pages.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
fixups.obj: fixups.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
info.obj: info.c lx.h
	wpp386 -zq -bt=dos32a $< -i=$CURSES
a.obj: a.c 
	wpp386 -zq -bt=dos32a $< -i=$CURSES
b.obj: b.c 
	wpp386 -d2 -zq -bt=dos32a $< -i=$CURSES

clean: .SYMBOLIC
	del *.obj
	del lxedit.exe
	del a.exe
	del b.exe
