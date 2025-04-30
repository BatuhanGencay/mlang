CXX = clang++
CXXFLAGS = `llvm-config --cxxflags` -std=c++17
LDFLAGS = `llvm-config --ldflags --system-libs --libs all`
SRC = src/lexer.cpp src/parser.cpp src/ast.cpp src/generator.cpp src/main.cpp
OUT = might

all:
	$(CXX) $(SRC) $(CXXFLAGS) $(LDFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)
