# Compiler
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -pedantic

# Targets
all: robotwarz test_robot

RobotBase.o: RobotBase.cpp RobotBase.h
	$(CXX) $(CXXFLAGS) -c RobotBase.cpp

test_robot: test_robot.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) test_robot.cpp RobotBase.o -ldl -o test_robot

# Main arena/game executable
robotwarz: Arena.cpp RobotBase.o
	$(CXX) $(CXXFLAGS) Arena.cpp RobotBase.o -ldl -o robotwarz

clean:
	rm -f *.o test_robot robotwarz *.so
