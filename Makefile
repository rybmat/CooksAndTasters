TARGET=CooksAndTasters
src=alg_v1.cpp
K=15
S=4
P=4
Z=3

all:
	mpic++ -o $(TARGET) $(src)

run:
	mpirun -np 50 ./$(TARGET) $K $S $P $Z

clean:
	rm $(TARGET)
