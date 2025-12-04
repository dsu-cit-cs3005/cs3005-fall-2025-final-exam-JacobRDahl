# Compiler
#CXX = g++
#CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Targets
#all: test_robot

#RobotBase.o: RobotBase.cpp RobotBase.h
#	$(CXX) $(CXXFLAGS) -c RobotBase.cpp

#test_robot: test_robot.cpp RobotBase.o
#	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot

#clean:
#	rm -f *.o test_robot *.so

# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Source files
SRC = RobotBase.cpp Arena.cpp PlayingBoard.cpp RobotWarz.cpp RobotList.cpp
OBJ = $(SRC:.cpp=.o)

# Targets
all: robotwarz test_robot

RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -fPIC -c RobotBase.cpp

test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot

robotwarz: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -ldl -o robotwarz

clean:
	rm -f *.o *.so test_robot robotwarz
