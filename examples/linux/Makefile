# JinWo VecDB Linux Demo Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c17 -I../../include
LDFLAGS = -L../../build -ljw_vecdb -pthread -lm

TARGET = linux_demo
SRC = linux_demo.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

build:
	cd ../.. && mkdir -p build && cd build && cmake .. && make

install: $(TARGET)
	install -d $(DESTDIR)/usr/local/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/local/bin/
