if VPATH_BUILD
# We need this so that generated sources can find non-generated headers and
# non-generated sources can find generated headers in a VPATH build from SVN.
INCLUDES += -I$(top_srcdir)/languages -Ilanguages
endif

noinst_HEADERS +=\
	languages/steminternal.h

snowball_algorithms =\
	languages/armenian.sbl\
	languages/basque.sbl\
	languages/catalan.sbl\
	languages/danish.sbl\
	languages/dutch.sbl\
	languages/english.sbl\
	languages/finnish.sbl\
	languages/french.sbl\
	languages/german2.sbl\
	languages/german.sbl\
	languages/hungarian.sbl\
	languages/italian.sbl\
	languages/kraaij_pohlmann.sbl\
	languages/lovins.sbl\
	languages/norwegian.sbl\
	languages/porter.sbl\
	languages/portuguese.sbl\
	languages/romanian.sbl\
	languages/russian.sbl\
	languages/spanish.sbl\
	languages/swedish.sbl\
	languages/turkish.sbl

snowball_built_sources =\
	$(snowball_algorithms:.sbl=.cc)\
	$(snowball_algorithms:.sbl=.h)

snowball_sources =\
	languages/compiler/space.c\
	languages/compiler/tokeniser.c\
	languages/compiler/analyser.c\
	languages/compiler/generator.c\
	languages/compiler/driver.c

snowball_headers =\
	languages/compiler/header.h\
	languages/compiler/syswords.h\
	languages/compiler/syswords2.h

EXTRA_DIST += $(snowball_sources) $(snowball_headers) $(snowball_algorithms) $(snowball_built_sources)\
	languages/dir_contents\
	languages/Makefile\
	languages/allsnowballheaders.h

if MAINTAINER_MODE
$(snowball_built_sources): languages/snowball $(snowball_algorithms)

languages/snowball: $(snowball_sources) $(snowball_headers)
	$(CC_FOR_BUILD) -o languages/snowball -DDISABLE_JAVA `for f in $(snowball_sources) ; do test -f $$f && echo $$f || echo $(srcdir)/$$f ; done`

.sbl.cc:
	languages/snowball $< -o `echo $@|sed 's!\.cc$$!!'` -c++ -u -n InternalStem`echo $<|sed 's!.*/\(.\).*!\1!'|tr a-z A-Z``echo $<|sed 's!.*/.!!;s!\.sbl!!'` -p SnowballStemImplementation

.sbl.h:
	languages/snowball $< -o `echo $@|sed 's!\.h$$!!'` -c++ -u -n InternalStem`echo $<|sed 's!.*/\(.\).*!\1!'|tr a-z A-Z``echo $<|sed 's!.*/.!!;s!\.sbl!!'` -p SnowballStemImplementation

languages/allsnowballheaders.h: languages/generate-allsnowballheaders languages/Makefile.mk
	languages/generate-allsnowballheaders $(snowball_built_sources)

BUILT_SOURCES += $(snowball_built_sources)\
	languages/allsnowballheaders.h
CLEANFILES += languages/snowball
endif

lib_src += $(snowball_built_sources)\
	languages/stem.cc\
	languages/steminternal.cc\
	languages/language_autodetect.cc
