## Question 2
```
mpicc 2_blocking.c -o 2_b -lm
mpiexec -np 9 -mca btl ^openib -hostfile hosts ./2_b

mpicc 2_non_blocking.c -o 2_b -lm
mpiexec -np 9 -mca btl ^openib -hostfile hosts ./2_b
```

To change problem size:
line7:2_blocking.c
line7:2_non_blocking.c

## Question 3
```
// Run serial program
mpicc image_processing_serial.c -o img_prc
mpiexec -np 1 -mca btl ^openib -hostfile hosts ./img_prc

// Run parallel program
mpicc image_processing_parallel.c -o img_prc
mpiexec -np 3 -mca btl ^openib -hostfile hosts ./img_prc
```
To change image file: 
line47:image_processing_parallel.c
line30:image_processing_serial.c

To change output filename:
line106:image_processing_parallel.c
line37:image_processing_serial.c

## Question 4
```
rpcgen –C –a calculator.x

make -f Makefile.calculator

//Run server code (on cspro2)
./calculator_server

//Run client code (on cspro4)
./calculator_client cspro2.sogang.ac.kr
2+3*5+2**3
```
