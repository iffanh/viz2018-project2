
CXX = gcc

LIBS = -lglut -lGLU -lGL -lGLEW -lstdc++
PROGRAM = main
OUTPUT = project2
#_1D
#Kilde


$(PROGRAM): $(PROGRAM).cpp 
		$(CXX) $(PROGRAM).cpp -o $(OUTPUT) $(LIBS)
#nrutil.c 
#nrutil.h nrutil.c
clean: 
		rm -f *.o