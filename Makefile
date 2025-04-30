CXX = clang++
CXXFLAGS = `llvm-config --cxxflags` -std=c++17
LDFLAGS = `llvm-config --ldflags --system-libs --libs all`
SRC = lexer.cpp parser.cpp ast.cpp generator.cpp main.cpp
OUT = might

all:
	$(CXX) $(SRC) $(CXXFLAGS) $(LDFLAGS) -o $(OUT)

clean:
	rm -f $(OUT)
