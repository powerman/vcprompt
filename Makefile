
CFLAGS = -Wall -Wextra -Wno-unused-parameter -g -O2

headers = $(wildcard src/*.h)
sources = $(wildcard src/*.c)
objects = $(subst .c,.o,$(sources))

vcprompt: $(objects)
	$(CC) -o $@ $(objects)

# Maximally pessimistic view of header dependencies.
$(objects): $(headers)

.PHONY: check check-simple check-hg check-git
check: check-simple check-hg check-git

hgrepo = tests/hg-repo.tar
gitrepo = tests/git-repo.tar

check-simple: vcprompt
	cd tests && ./test-simple

check-hg: vcprompt $(hgrepo)
	cd tests && ./test-hg

$(hgrepo): tests/setup-hg
	cd tests && ./setup-hg

check-git: vcprompt $(gitrepo)
	cd tests && ./test-git

$(gitrepo): tests/setup-git
	cd tests && ./setup-git

clean:
	rm -f $(objects) vcprompt

DESTDIR =
PREFIX = /usr/local
.PHONY: install
install: vcprompt
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 vcprompt $(DESTDIR)$(PREFIX)/bin
