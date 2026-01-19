# Set the compiler and flags
CXX = g++
CXXFLAGS = -w -std=c++17 -I$(CONDA_PREFIX)/include
LDFLAGS = -L$(CONDA_PREFIX)/lib
LDLIBS = -lboost_filesystem -lboost_system -lxcdf  `root-config --cflags --libs`

# Set the output binary name
TARGET = xcdf2root

# Set the installation directory
INSTALL_DIR = $(HOME)/bin

# Source files
SRC = xcdf2root.cpp

# Object files
OBJ = $(SRC:.cpp=.o)

# Define the rules

# Rule to build the target
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS) $(LDLIBS) 

# Rule to build object files from source files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Rule to install the target to $(HOME)/bin
install: $(TARGET)
	cp $(TARGET) $(INSTALL_DIR)/

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJ)

# Phony targets
.PHONY: clean install

