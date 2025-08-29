RUSTDIR = myrustcode
TARGET = main
RUSTLIB = $(RUSTDIR)/target/debug/libmyrustcode.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): main.cpp $(RUSTLIB)
	g++ -std=c++11 -o $@ $< -L$(RUSTDIR)/target/debug -lmyrustcode

$(RUSTLIB): $(RUSTDIR)/src/generated.rs
	cd $(RUSTDIR) && cargo build

$(RUSTDIR)/src/generated.rs: $(RUSTDIR)/main.zng
	cd $(RUSTDIR) && zngur generate main.zng

clean:
	rm -f $(TARGET)
	cd $(RUSTDIR) && cargo clean
