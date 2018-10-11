# Makefile for TuxPuck , Copyright Jacob Kroon 2001-2002
NAME		= tuxpuck
VERSION		= 0.9.0-prealpha1
CC		= m68k-amigaos-gcc
CFLAGS		+= -noixemul -Os -Wall -Wno-unused -fomit-frame-pointer -D_VERSION=\"$(VERSION)\" -D_DEBUG=1
CSOURCES	= tuxpuck.c video.c audio.c menu.c sprite.c font.c timer.c \
		  board.c entity.c glass.c scoreboard.c player.c zoom.c png.c \
		  jpg.c intro.c tux.c arcana.c buff.c a1d2.c smasher.c lex.c \
		  morth.c sam.c coler.c tin.c 
INCLUDES	= tuxpuck.h video.h audio.h font.h
	   	  

#############################################################

OBJS=$(CSOURCES:.c=.o)
ifdef COMSPEC
  OBJS += w32icon.o
endif

%.o	: %.c
	$(CC) $(CFLAGS) -I/opt/amiga/SDL_image-pack/include -I/opt/amiga/libvorbis/include  -I/opt/amiga/SDL_image-pack/include/SDL -I/opt/amiga/include/SDL -I/opt/amiga/SDL_mixer/include -c -o $@ $<

$(NAME) : $(OBJS)
	cd data; $(MAKE)
	#$(CC) $(CFLAGS) $(OBJS) data/libdata.a -L/data/libvorbis/lib -L/opt/amiga/SDL_image-pack/lib -L/opt/amiga/zlib-package/lib /root/amiga-gcc/build/newlib/newlib/libm.a -lpng \
	#-ljpeg -lz -lvorbisfile -lvorbis -logg -lSDL -ldebug -o $(NAME)
	m68k-amigaos-gcc  -noixemul -Os -Wall -Wno-unused -fomit-frame-pointer -D_VERSION=\"0.9.0-prealpha1\" tuxpuck.o video.o audio.o menu.o sprite.o font.o timer.o board.o entity.o glass.o scoreboard.o player.o zoom.o png.o jpg.o intro.o tux.o arcana.o buff.o a1d2.o smasher.o lex.o morth.o sam.o coler.o tin.o data/libdata.a -L/opt/amiga/libvorbis/lib -L/opt/amiga/SDL_image-pack/lib -L/opt/amiga/zlib-package/lib  -lpng -ljpeg -lz -lvorbisfile /root/amiga-gcc/build/newlib/newlib/libm.a -lvorbis -logg -L/opt/amiga/SDL_mixer/lib -lSDL_mixer -lSDL -ldebug -o tuxpuck

w32icon.o : data/icons/tuxpuck.ico
	echo AppIcon ICON "data/icons/tuxpuck.ico" > temp.rc
	windres -i temp.rc -o w32icon.o
	rm temp.rc

clean :
	cd utils; $(MAKE) clean;
	cd data; $(MAKE) clean;
	rm -f *~ $(OBJS) $(NAME)

indent :
	cd utils; $(MAKE) indent;
	indent -br -brs -sob -ce -c50 -npsl -npcs $(CSOURCES) $(INCLUDES)
	rm -f *~

dist :
	$(MAKE) clean
	mkdir $(NAME)-$(VERSION)
	cp $(CSOURCES) $(INCLUDES) readme.txt todo.txt bugs.txt thanks.txt \
	  COPYING Makefile $(NAME)-$(VERSION)
	cp -R man utils data $(NAME)-$(VERSION)
	tar -cf $(NAME)-$(VERSION).tar $(NAME)-$(VERSION)
	tar -f $(NAME)-$(VERSION).tar --delete \
	  `tar -tf $(NAME)-*.tar | grep -w -e ".svn/"`
	gzip -9 $(NAME)-$(VERSION).tar
	rm -Rf $(NAME)-$(VERSION)

install : $(NAME)
	install -d $(DESTDIR)/usr/bin
	install -d $(DESTDIR)/usr/man/man6
	install -m755 $(NAME) $(DESTDIR)/usr/bin
	install -m644 man/$(NAME).6.gz $(DESTDIR)/usr/man/man6
