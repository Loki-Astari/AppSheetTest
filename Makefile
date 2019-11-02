
CXXFLAGS	+= -std=c++17 -Ithird/ThorsSerializer/
CXXFLAGS	+= -Wall -Wextra -Wstrict-aliasing -pedantic -Werror -Wunreachable-code -Wno-long-long -Wno-unknown-pragmas

SRC			= $(wildcard *.cpp)
OBJ			= $(patsubst %.cpp, %.o, $(SRC))

appTest:	$(OBJ)
	$(CXX) -O3 -o $@ $(OBJ)

clean:
	$(RM)	$(OBJ) appTest
