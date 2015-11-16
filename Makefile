#Makefile
#heth@mercantec.dk
CC=gcc
CFLAGS=-std=c99 -pedantic
AOUT=ht
 
OBJECTS=main.o ht_malloc-pedantic.o

 
$(AOUT): $(OBJECTS)
	@echo "Building..."                    # This line must start with a <TAB>
	$(CC) $(CFLAGS) $(OBJECTS) -o $(AOUT)   # This line must start with a <TAB>
 
clean:
	@echo "Cleaning binaries"              # This line must start with a <TAB>
	/bin/rm $(OBJECTS) $(AOUT)             # This line must start with a <TAB>
#dependencies

