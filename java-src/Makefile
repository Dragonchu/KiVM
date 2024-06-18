JAVAC := javac

DIR := $(shell pwd)
SRC := $(wildcard com/imkiva/kivm/*.java)
OBJ := $(patsubst %.java,../java-out/%.class,$(SRC))

all: $(OBJ)
	@echo " Done"

../java-out/%.class: %.java
	@echo "[Compiling] $<"
	@mkdir -p $(@D)
	@$(JAVAC) -d $(@D) -encoding utf-8 $<
	@touch $@

clean:
	@echo "[Cleaning]"
	@rm -rf ../java-out