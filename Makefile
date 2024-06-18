CC=gcc
CFLAGS=-g -Wall -Wextra -O2 -D_FORTIFY_SOURCE=2 -pipe -march=native -Werror=format-security -fpie -fstack-protector-strong -fcf-protection -pie -fPIC -fno-plt $(shell pkg-config --cflags libmicrohttpd) $(shell bash ./libphp/scripts/php-config --includes)
LDFLAGS=$(shell pkg-config --libs libmicrohttpd) -L$(shell bash ./libphp/scripts/php-config --prefix)/lib -lphp -Wl,-rpath=$(shell bash ./libphp/scripts/php-config --prefix)/lib -pie


#CFLAGS=-march=x86-64 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=3 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -g -ffile-prefix-map=/build/php/src=/usr/src/debug/php -flto=auto
#LDFLAGS=-Wl,-O1 -Wl,--sort-common -Wl,--as-needed -Wl,-z,relro -Wl,-z,now -Wl,-z,pack-relative-relocs -flto=auto
#CXXFLAGS=-march=x86-64 -mtune=generic -O2 -pipe -fno-plt -fexceptions -Wp,-D_FORTIFY_SOURCE=3 -Wformat -Werror=format-security -fstack-clash-protection -fcf-protection -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -Wp,-D_GLIBCXX_ASSERTIONS -g -ffile-prefix-map=/build/php/src=/usr/src/debug/php -flto=auto

# Name of our executable
TARGET=otter

# Where the binary is stored
BUILD_DIR=build

# Where all the C files are stored
SOURCE_DIR=src

# Find all the C files we want to compile
SRCS=$(shell find $(SOURCE_DIR) -name '*.c')

# Puts all object files into the BUILD_DIR (src/file.c -> build/file.c.o)
OBJS=$(SRCS:%=$(BUILD_DIR)/%.o)

# Puts all dependencis into the BUILD_DIR (build/file.c.o -> build/file.c.d)
DEPS=$(OBJS:.o=.d)

# Any folder in SOURCE_DIR will be included (prefixed with -I)
INC_DIR=$(shell find $(SOURCE_DIR) -type d)
INC_FLAGS=$(addprefix -I,$(INC_DIR))

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: flags
flags:
	@echo "CC: ${CC}"
	@echo "CFLAGS: ${CFLAGS}"
	@echo "LDFLAGS: ${LDFLAGS}"

gdb: clean $(BUILD_DIR)/$(TARGET)
	gdb $(BUILD_DIR)/$(TARGET)

info: clean $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET) -i

run: clean $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/$(TARGET)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# include the .d makefiles
-include $(DEPS)
