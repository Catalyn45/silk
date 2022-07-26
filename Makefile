PROJNAME=silk

CC=clang
CFLAGS=-fPIC -ggdb -O0 -Wall -Wextra -Werror -Winline

LIBS=
TEST_LIBS=

C=$(wildcard src/*.c) $(wildcard src/*/*.c)
TEST_C=$(wildcard tests/*.c) $(wildcard tests/*/*.c)

OBJ=$(patsubst %,%.o,$(basename $(C)))
TEST_OBJ=$(filter-out src/main.o,$(patsubst %,%.o,$(basename $(TEST_CPP))) $(OBJ))

default: $(PROJNAME)

%.o: %.c
	@echo -e "\033[0;32mCompiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(PROJNAME): $(OBJ)
	@echo -e "\033[0;36mLinking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	@echo -e "\033[0;35mSrc done"

test: $(TEST_OBJ)
	@echo -e "\033[0;36mLinking $@"
	@$(CC) $(CFLAGS) -o $(PROJNAME)_test $^ $(LIBS) $(TEST_LIBS)
	@echo -e "\033[0;35mTest done"

all: $(PROJNAME) test
	@echo -e "\033[0;35mAll done"

clean:
	@echo -e "\033[1;33mCleaning up"
	@rm $(PROJNAME) -f
	@rm $(OBJ) -f
