
CFLAGS = -Wall -g -O0

headers = $(wildcard src/*.h)
sources = $(wildcard src/*.c)
objects = $(subst .c,.o,$(sources))

vcprompt: $(objects)
	$(CC) -o $@ $(objects)

# Maximally pessimistic view of header dependencies.
$(objects): $(headers)

.PHONY: check
check: vcprompt
	./tests/test-simple

clean:
	rm -f $(objects) vcprompt
