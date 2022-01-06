SHELL = /bin/sh
SYSTEM = $(shell uname)
C++ = g++
CC = gcc
DFLAGS = -DGHOST_MYSQL
OFLAGS = -O3
LFLAGS =  -L. -L./bncsutil -L./StormLib -L./zlib-ng  -Wl,-Bstatic -lbncsutil_static -lz -lstorm  -Wl,-Bdynamic -lpthread -ldl -lbz2 -lmysqlclient -lgmp -lsqlite3 -lssl -lcrypto
CFLAGS = -g3 -Wall

ifeq ($(SYSTEM),Darwin)
DFLAGS += -D__APPLE__
OFLAGS += -flat_namespace
else
LFLAGS += -lrt
endif

ifeq ($(SYSTEM),FreeBSD)
DFLAGS += -D__FREEBSD__
endif

ifeq ($(SYSTEM),SunOS)
DFLAGS += -D__SOLARIS__
LFLAGS += -lresolv -lsocket -lnsl
endif

CFLAGS += $(OFLAGS) $(DFLAGS) -I. -I./bncsutil/src/bncsutil -I./StormLib/src 

ifeq ($(SYSTEM),Darwin)
CFLAGS += -I./mysql/include/
endif

OBJS = lia.o discord.o bncsutilinterface.o bnet.o bnetprotocol.o bnlsclient.o bnlsprotocol.o commandpacket.o config.o crc32.o csvparser.o game.o game_admin.o game_base.o gameplayer.o gameprotocol.o gameslot.o ghost.o database/ghostdb.o database/ghostdbmysql.o database/ghostdbsqlite.o gpsprotocol.o language.o map.o packed.o replay.o savegame.o socket.o stats/stats.o stats/statsdota.o stats/statsw3mmd.o stats/statslia.o stats/statsdebug.o util.o 
COBJS = 
PROGS = ./ghost++

all: $(OBJS) $(COBJS) $(PROGS)

./ghost++: $(OBJS) $(COBJS)
	$(C++) -o ./ghost++ $(OBJS) $(COBJS) $(LFLAGS)

clean:
	rm -f $(OBJS) $(COBJS) $(PROGS)

$(OBJS): %.o: %.cpp
	$(C++) -o $@ $(CFLAGS) -c $<

$(COBJS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

./ghost++: $(OBJS) $(COBJS)

all: $(PROGS)

bncsutilinterface.o: ghost.h includes.h util.h bncsutilinterface.h
bnet.o: ghost.h includes.h util.h config.h language.h socket.h commandpacket.h database/ghostdb.h bncsutilinterface.h bnlsclient.h bnetprotocol.h bnet.h map.h packed.h savegame.h replay.h gameprotocol.h game_base.h
bnetprotocol.o: ghost.h includes.h util.h bnetprotocol.h
bnlsclient.o: ghost.h includes.h util.h socket.h commandpacket.h bnlsprotocol.h bnlsclient.h
bnlsprotocol.o: ghost.h includes.h util.h bnlsprotocol.h
commandpacket.o: ghost.h includes.h commandpacket.h
config.o: ghost.h includes.h config.h
crc32.o: ghost.h includes.h crc32.h
csvparser.o: csvparser.h
game.o: lia.h discord.h ghost.h includes.h util.h config.h language.h socket.h database/ghostdb.h bnet.h map.h packed.h savegame.h gameplayer.h gameprotocol.h game_base.h game.h stats/stats.h stats/statsdota.h stats/statsw3mmd.h stats/statslia.h stats/statsdebug.h
game_admin.o: ghost.h includes.h util.h config.h language.h socket.h database/ghostdb.h bnet.h map.h packed.h savegame.h replay.h gameplayer.h gameprotocol.h game_base.h game_admin.h
game_base.o: lia.h ghost.h includes.h util.h config.h language.h socket.h database/ghostdb.h bnet.h map.h packed.h savegame.h replay.h gameplayer.h gameprotocol.h game_base.h next_combination.h
gameplayer.o: ghost.h includes.h util.h language.h socket.h commandpacket.h bnet.h map.h gameplayer.h gameprotocol.h gpsprotocol.h game_base.h
gameprotocol.o: ghost.h includes.h util.h crc32.h gameplayer.h gameprotocol.h game_base.h
gameslot.o: ghost.h includes.h gameslot.h
ghost.o: discord.o ghost.h includes.h util.h crc32.h csvparser.h config.h language.h socket.h database/ghostdb.h database/ghostdbsqlite.h database/ghostdbmysql.h bnet.h map.h packed.h savegame.h gameplayer.h gameprotocol.h gpsprotocol.h game_base.h game.h game_admin.h
database/ghostdb.o: ghost.h includes.h util.h config.h database/ghostdb.h
database/ghostdbmysql.o: ghost.h includes.h util.h config.h database/ghostdb.h database/ghostdbmysql.h
database/ghostdbsqlite.o: ghost.h includes.h util.h config.h database/ghostdb.h database/ghostdbsqlite.h
gpsprotocol.o: ghost.h util.h gpsprotocol.h
language.o: ghost.h includes.h config.h language.h
map.o: ghost.h includes.h util.h crc32.h config.h map.h
packed.o: ghost.h includes.h util.h crc32.h packed.h
replay.o: ghost.h includes.h util.h packed.h replay.h gameprotocol.h
savegame.o: ghost.h includes.h util.h packed.h savegame.h
socket.o: ghost.h includes.h util.h socket.h
stats/stats.o: ghost.h includes.h stats/stats.h
stats/statsdota.o: ghost.h includes.h util.h database/ghostdb.h gameplayer.h gameprotocol.h game_base.h stats/stats.h stats/statsdota.h
stats/statsw3mmd.o: ghost.h includes.h util.h database/ghostdb.h gameprotocol.h game_base.h stats/stats.h stats/statsw3mmd.h
stats/statslia.o: ghost.h includes.h util.h database/ghostdb.h gameplayer.h gameprotocol.h game_base.h stats/stats.h stats/statslia.h
stats/statsdebug.o: ghost.h includes.h util.h database/ghostdb.h gameplayer.h gameprotocol.h game_base.h stats/stats.h stats/statsdebug.h
util.o: ghost.h includes.h util.h
discord.o : discord.h
lia.o : lia.h
