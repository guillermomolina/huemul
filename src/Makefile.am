bin_PROGRAMS = huemul
huemul_SOURCES = alloc.c gtk.c huemul.c image.c primitive.c print.c send.c \
	specialMethod.c

# set the include path found by configure
INCLUDES= $(all_includes) @GTK_CFLAGS@

# the library search path.
huemul_LDFLAGS = $(all_libraries) @GTK_LIBS@ -rdynamic

noinst_HEADERS = huemul.h
huemul_LDADD = -lreadline -ldl -lpthread
