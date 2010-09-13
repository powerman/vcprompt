
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g -O2

headers = $(wildcard src/*.h)
sources = $(wildcard src/*.c)
objects = $(subst .c,.o,$(sources))

vcprompt: $(objects)
	$(CC) -o $@ $(objects)

# Maximally pessimistic view of header dependencies.
$(objects): $(headers)

.PHONY: check
check: vcprompt
	cd tests && ./test-simple

gitrepo = tests/git-repo.tar

check-git: vcprompt $(gitrepo)
	cd tests && ./test-git

$(gitrepo): tests/setup-git
	cd tests && ./setup-git

# target check-all requires that all supported VC tools be
# installed
check-all: check check-git

clean:
	rm -f $(objects) vcprompt
