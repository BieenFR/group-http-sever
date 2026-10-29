CC = cc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
LDFLAGS =
LDLIBS =
PYTHON = python3
# POSIX-style make: explicit objects, no GNU wildcard/pattern rules.
# OmniOS may require: make LDLIBS="-lsocket -lnsl"
# Add libmagic flags only after verifying the target system's library.
OBJECTS = src/main.o src/config.o src/server.o src/request.o src/files.o src/response.o src/cgi.o src/logger.o
all: sws
sws: $(OBJECTS)
	$(CC) $(LDFLAGS) -o sws $(OBJECTS) $(LDLIBS)
$(OBJECTS): include/http.h
.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
check: sws
	./sws -h
	$(PYTHON) tests/check_assets.py
test-bien: check
	$(PYTHON) tests/test_bien.py
test-request: check
	$(PYTHON) tests/test_request.py
test: check
	$(PYTHON) tests/http_acceptance.py
clean:
	rm -f $(OBJECTS) sws
