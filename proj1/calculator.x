struct custom_string {
  char str<100>;
};

struct custom_response {
  int result;
  int is_valid;
};

program CAL_PROG {
  version CAL_VERS {
    custom_response CALCULATE(custom_string) = 1;
  } = 1;
} = 0x31111111;