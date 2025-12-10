CXX = g++
CXXFLAGS = -O3 -march=native -DNDEBUG

SRC = hrac.cc main.cc
OUT = hrac

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)
