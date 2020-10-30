struct custom_string {
  char str<100>;
};

program CAL_PROG {
  version CAL_VERS {
    int CALCULATE(custom_string) = 1;
  } = 1;
} = 0x31111111;