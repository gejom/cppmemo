CC                = g++
CFLAGS_COMMON     = -I".." -std=c++11 -Wall
CFLAGS_RELEASE    = $(CFLAGS_COMMON) -O3
CFLAGS_DEBUG      = $(CFLAGS_COMMON) -O0 -g
CFLAGS            = $(CFLAGS_RELEASE)
LDFLAGS           = -lpthread
INCLUDES          = ../cppmemo.hpp

all: fibonacci knapsack matrix_chain cycle_check custom_container
	
fibonacci: fibonacci.o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS)
	
knapsack: knapsack.o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS)

matrix_chain: matrix_chain.o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS)

cycle_check: cycle_check.o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS)

custom_container: custom_container.o
	$(CC) -o $@ $@.o $(CFLAGS) $(LDFLAGS)

%.o : %.cpp $(INCLUDES)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	@rm -f fibonacci.o
	@rm -f knapsack.o
	@rm -f matrix_chain.o
	@rm -f cycle_check.o
	@rm -f custom_container.o
	@rm -f fibonacci
	@rm -f knapsack
	@rm -f matrix_chain
	@rm -f cycle_check
	@rm -f custom_container
