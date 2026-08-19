# tests/Makefile — host-side unit tests for 3ds-fast-uninstall
# Run with:  make -C tests
# or from the repo root:  make test

CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-parameter \
           -I../source \
           -Ishims \
           -Ivendor \
           -DCONFIG_PATH=\"test_config.ini\" \
           -DDEFAULT_BACKUP_PATH=\"/test/backups\"

STUBS   := stubs/globals_stub.c stubs/host_stubs.c

BINS    := test_utils test_config test_titles test_backup test_install

.PHONY: all clean

all: $(BINS)
	@echo "--- Running test_utils ---"
	@./test_utils
	@echo "--- Running test_config ---"
	@./test_config
	@echo "--- Running test_titles ---"
	@./test_titles
	@echo "--- Running test_backup ---"
	@./test_backup
	@echo "--- Running test_install ---"
	@./test_install
	@echo "=== All test suites completed ==="

test_utils: test_utils.c $(STUBS)
	$(CC) $(CFLAGS) -o $@ $^

test_config: test_config.c $(STUBS)
	$(CC) $(CFLAGS) -o $@ $^

test_titles: test_titles.c $(STUBS)
	$(CC) $(CFLAGS) -o $@ $^

test_backup: test_backup.c $(STUBS)
	$(CC) $(CFLAGS) -o $@ $^

test_install: test_install.c $(STUBS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BINS) test_config.ini
	rm -rf /tmp/3dsfu_br_test /tmp/3dsfu_br_restore /tmp/3dsfu_install_test
