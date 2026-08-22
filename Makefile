CC = gcc
WINDRES = windres
CFLAGS = -O2 -Wall -Wextra -D_UNICODE -DUNICODE -municode
LDFLAGS_GUI = -mwindows -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lgdi32
LDFLAGS_CLI = -luser32 -lshell32 -lcomdlg32 -ladvapi32 -lgdi32

SRCS = src/main.c src/strings.c src/config.c src/hotkey.c src/process_monitor.c src/runner.c src/dpi_utils.c src/ui_main.c src/ui_settings.c
OBJS = $(SRCS:.c=.o)
RES_OBJ = res/resource.o
TARGET = FanControlHotkey.exe
TEST_TARGET = tests/test_main.exe
TEST_SRCS = tests/test_main.c src/strings.c src/config.c src/hotkey.c src/process_monitor.c src/runner.c src/dpi_utils.c

all: $(TARGET)

$(RES_OBJ): res/resource.rc res/resource.h res/app.manifest res/icon.ico
	$(WINDRES) res/resource.rc -O coff -o $(RES_OBJ)

$(TARGET): $(OBJS) $(RES_OBJ)
	$(CC) $(CFLAGS) $(OBJS) $(RES_OBJ) -o $(TARGET) $(LDFLAGS_GUI)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(LDFLAGS_CLI)

clean:
	rm -f src/*.o res/*.o $(TARGET) $(TEST_TARGET)

.PHONY: all test clean
