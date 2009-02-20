PREFIX ?= /usr

OPTS=-Wall -Wextra -ansi -pedantic-errors -Wconversion $(CXXFLAGS)
DICTIONARY_PATH=$(PREFIX)/share/jmdict
DICTIONARY_NAME="\"$(DICTIONARY_PATH)/database\""
BINDIR=${DESTDIR}${PREFIX}/bin

all:	jmdict jmdict_import
clean:
	@echo cleaning up...
	@rm -f jmdict jmdict_import jmdict.o jmdict_import.o kana2romaji.o
kana2romaji.o:	kana2romaji.cpp kana2romaji.h
	$(CXX) $(OPTS) -c -o kana2romaji.o kana2romaji.cpp
jmdict:	jmdict.o kana2romaji.o
	$(CXX) $(OPTS) -o jmdict jmdict.o kana2romaji.o -lsqlite3
jmdict.o:	jmdict.cpp sqlite.h
	$(CXX) $(OPTS) -c -o jmdict.o jmdict.cpp -DDICTIONARY_PATH=$(DICTIONARY_NAME)
jmdict_import:	jmdict_import.o kana2romaji.o
	$(CXX) $(OPTS) -o jmdict_import jmdict_import.o kana2romaji.o -lsqlite3 -lexpat

jmdict_import.o:	jmdict_import.cpp sqlite.h xmlparser.h kana2romaji.h
	$(CXX) $(OPTS) -c -o jmdict_import.o jmdict_import.cpp -DDICTIONARY_PATH=$(DICTIONARY_NAME)

install:
	install -d ${DESTDIR}$(DICTIONARY_PATH)
	install -d $(BINDIR)
	install jmdict $(BINDIR)
	install jmdict_import $(BINDIR)
