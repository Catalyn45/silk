PROJNAME=sylk
TESTNAME=$(PROJNAME)_test
LIBNAME=lib$(PROJNAME)

CC=clang
CXX=clang++

CFLAGS=-fPIC -Wall -Wextra -Werror -Winline -MD -g

LIBS=
TEST_LIBS= -lgtest -lgtest_main

C=$(shell find ./src -name "*.c")
TEST_CPP=$(shell find ./test -name "*.cpp")
TEST_OBJ=$(patsubst %,obj/%.o,$(basename $(TEST_CPP)))

ALL_OBJ=$(patsubst %,obj/%.o,$(basename $(C)))
OBJ=$(filter-out obj/./src/main.o, $(ALL_OBJ))
MAIN_OBJ = obj/./src/main.o

default: all

obj/./src/%.o: src/%.c
	@mkdir -p $(@D)
	@echo -e "\033[0;32mCompiling $<"
	@$(CC) $(CFLAGS) -c $< -o $@

obj/./test/%.o: test/%.cpp
	@mkdir -p $(@D)
	@echo -e "\033[0;32mCompiling $<"
	@$(CXX) $(CFLAGS) -I . -c $< -o $@

$(PROJNAME): $(OBJ) $(MAIN_OBJ)
	@echo -e "\033[0;36mLinking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(TESTNAME): $(TEST_OBJ) $(OBJ)
	@echo -e "\033[0;36mLinking $@"
	@$(CXX) $(CFLAGS) -o $(TESTNAME) $^ $(LIBS) $(TEST_LIBS)

$(LIBNAME).a: $(OBJ)
	@echo -e "\033[0;36mLinking $@"
	@ar rcs $@ $^

$(LIBNAME).so: $(OBJ)
	@echo -e "\033[0;36mLinking $@"
	@$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)

binary: $(PROJNAME)
	@echo -e "\033[0;35mBinary done"

test: $(TESTNAME)
	@echo -e "\033[0;37mTest done"

lib: $(LIBNAME).so $(LIBNAME).a
	@echo -e "\033[0;35mLib done"

all: lib binary 
	@echo -e "\033[0;37mAll done"

clean:
	@echo -e "\033[1;33mCleaning up"
	@rm $(PROJNAME) -f
	@rm $(TESTNAME) -f
	@rm $(LIBNAME).so -f
	@rm $(LIBNAME).a -f
	@rm $(OBJ) -f
	@rm $(MAIN_OBJ) -f
	@rm $(patsubst %.o, %.d, $(MAIN_OBJ))
	@rm $(patsubst %.o, %.d, $(OBJ))
	@rm $(TEST_OBJ) -f
	@rm $(patsubst %.o, %.d, $(TEST_OBJ))

-include $(OBJ:.o=.d)
-include $(MAIN_OBJ:.o=.d)
-include $(TEST_OBJ:.o=.d)
