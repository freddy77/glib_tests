GLIB_DIR := $(HOME)/work/glib/install
export LD_RUN_PATH=$(GLIB_DIR)/lib
CFLAGS := -I$(GLIB_DIR)/include/glib-2.0 -I$(GLIB_DIR)/lib/glib-2.0/include -L$(GLIB_DIR)/lib -lglib-2.0
#CFLAGS := $(shell pkg-config --cflags --libs glib-2.0)


all:: mainloop
	./mainloop

clean::
	rm -f mainloop

mainloop: mainloop.c Makefile
	gcc -Wall -Werror -O2 -ggdb $(CFLAGS) -o $@ $<

