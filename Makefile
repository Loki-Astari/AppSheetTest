
CXXFLAGS	+= -std=c++17 -Ithird/ThorsSerializer/ -I third/ThorsStream/
CXXFLAGS	+= -Wall -Wextra -Wstrict-aliasing -pedantic -Werror -Wunreachable-code -Wno-long-long -Wno-unknown-pragmas
LDFLAGS		+= -lcurl

SRC			= $(wildcard *.cpp)
OBJ			= $(patsubst %.cpp, %.o, $(SRC))

appTest:	$(OBJ)
	$(CXX) -O3 -o $@ $(OBJ) $(LDFLAGS)

clean:
	$(RM)	$(OBJ) appTest
