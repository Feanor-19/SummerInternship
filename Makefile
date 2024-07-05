CC=g++

ifdef SAN
SAN = -fsanitize=alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr,leak,address
else
SAN =
endif

# some of this flags are only for g++, not for gcc
CFLAGS  =  -D _DEBUG -ggdb3 -std=c++17 -Wall -Wextra -Weffc++                   \
      -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations         \
      -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported         \
      -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal             \
      -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2         \
      -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual     \
      -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow           \
      -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2       \
      -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types       \
      -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef         \
      -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix   \
      -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs   \
      -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector         \
      -fstrict-overflow -flto-odr-type-merging -fno-omit-frame-pointer          \
      -Wstack-usage=8192 -pie -fPIE -Werror=vla $(SAN)

OBJ = obj
SRC = src
BIN = bin
INC = inc

C_EXT   = .cpp
OBJ_EXT = .o

LIB_NAMES = sss_nss_idmap 

SOURCES  = $(wildcard $(SRC)/*$(C_EXT))
OBJFILES = $(patsubst $(SRC)/%,$(OBJ)/%,$(SOURCES:$(C_EXT)=$(OBJ_EXT)))
OUT      = $(BIN)/prog

LIB_SUBST = -l$(lib_name) 
LIBS      = $(foreach lib_name,$(LIB_NAMES),$(LIB_SUBST))

$(OUT) : $(OBJFILES)
	$(CC) -o $@ $(CFLAGS) $^ -I $(INC) $(LIBS)

$(OBJ)/%$(OBJ_EXT) : $(SRC)/%$(C_EXT)
	$(CC) -c $(CFLAGS) -o $@ $< -I $(INC)

.PHONY: run
run:
	$(OUT)

.PHONY: clean
clean:
	rm -f $(OBJFILES) $(OUT)

.PHONY: copy_to_TEST
copy_to_TEST:
	sudo cp bin/prog /usr/bin/TEST/prog