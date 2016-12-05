PROG := prob
CFLAGS := -g -O2 -fomit-frame-pointer -pthread
CC := gcc
INCLUDES := -I.
LIBS := -lpthread -lncursesw

.PHONY: clean pf_gen

$(PROG): main.o interactive.o preflop.o helper.o
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

main.o: main.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

intractive.o: interactive.c
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

preflop.o: preflop.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

helper.o: helper.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

preflop.c: pf_gen

people: players.c
	$(CC) $(INCLUDES) -I/usr/include/mysql $(CFLAGS) `mysql_config --libs` $(LIBS) players.c -o $@	

pf_gen:
	./pfgen.awk table.txt > preflop.c
	

clean:
	rm -f $(PROG) *.o
