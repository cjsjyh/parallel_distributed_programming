## Question 2
```
mpicc 2_blocking.c -o 2_b -lm
mpiexec -np 9 -mca btl ^openib -hostfile hosts ./2_b

mpicc 2_non_blocking.c -o 2_b -lm
mpiexec -np 9 -mca btl ^openib -hostfile hosts ./2_b
```

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