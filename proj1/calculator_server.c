/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */
#include <string.h>
#include "calculator.h"

//#define TRUE 1
//#define FALSE 0
enum OPERATOR {
  PLUS = 0,
  MINUS,
  MULT,
  DIV,
  POW
};
#define MAX_PRIORITY 2
int priority[] = {2, 2, 1, 1, 0};

int string_parse(char* str, int nums[], int* num_cnt, enum OPERATOR ops[], int* op_cnt);
int calculate(int nums[], int *num_cnt, enum OPERATOR ops[], int *op_cnt);
void pull_array(void* arr, int* count, int index, int type);
int custom_pow(int, int);
void printArray(int*, int);

custom_response *
calculate_1_svc(custom_string *argp, struct svc_req *rqstp)
{
	static custom_response  result;
  int nums[100], num_cnt, op_cnt, is_valid;
  enum OPERATOR ops[100];

  result.is_valid = 1;
  is_valid = string_parse(argp->str.str_val, nums, &num_cnt, ops, &op_cnt);
  // 1. number of operators and numbers doesn't match
  // 2. operators came in a row
  // 3. ended with a operator
  if(num_cnt != op_cnt + 1 || is_valid == FALSE){
    printf("Invalid operation\n");
    result.is_valid = 0;
    return &result;
  }
  result.result = calculate(nums, &num_cnt, ops, &op_cnt);

  printf("Result: %d\n", result.result);


	return &result;
}

int string_parse(char* str, int nums[], int* num_cnt, enum OPERATOR ops[], int* op_cnt){
  char temp[100];
  int last_op = -1, is_pow=FALSE, is_op=FALSE;
  *num_cnt = *op_cnt = 0;

  for(int i=0; i<strlen(str); i++){
    is_op = FALSE;
    switch (str[i]){
      case '+':
        is_op = TRUE;
        is_pow = FALSE;
        ops[(*op_cnt)++] = PLUS;
        break;
      case '-':
        is_op = TRUE;
        is_pow = FALSE;
        ops[(*op_cnt)++] = MINUS;
        break;
      case '/':
        is_op = TRUE;
        is_pow = FALSE;
        ops[(*op_cnt)++] = DIV;
        break;
      case '*':
        is_op = TRUE;
        // power
        if(i+1 < strlen(str) && str[i+1] == '*'){
          is_pow = TRUE;
          ops[(*op_cnt)++] = POW;
          i++;
        }
        // multiplication
        else {
          is_pow = FALSE;
          ops[(*op_cnt)++] = MULT;
        }
        break;
    }
    if(is_op){
      // operators came together
      if (i - last_op == 1)
        return FALSE;
      // operators came together before power
      else if(i - last_op == 2 && is_pow == TRUE)
        return FALSE;
      // operation ended with an operator
      else if(i == strlen(str)-1)
        return FALSE;
      strncpy(temp, &(str[last_op+1]), i-last_op-1);
      temp[i-last_op-1] = '\0';
      nums[(*num_cnt)++] = atoi(temp);
      last_op = i;
    }
  }
  strncpy(temp, &(str[last_op+1]), strlen(str)-last_op-1);
  temp[strlen(str)-last_op-1] = '\0';
  nums[(*num_cnt)++] = atoi(temp);

  return TRUE;
}

int calculate(int nums[], int *num_cnt, enum OPERATOR ops[], int *op_cnt){
  for(int i=0; i <= MAX_PRIORITY; i++){
    for(int j=0; j<*op_cnt; j++){
      if(priority[ops[j]] == i){
        switch(ops[j]){
          case PLUS:
            nums[j] += nums[j+1]; 
            break;
          case MINUS:
            nums[j] -= nums[j+1];
            break;
          case MULT:
            nums[j] *= nums[j+1];
            break;
          case DIV:
            nums[j] /= nums[j+1];
            break;
          case POW:
            nums[j] = custom_pow(nums[j], nums[j+1]);
            break;
        }
        pull_array(nums, num_cnt, j+1, 1);
        pull_array(ops, op_cnt, j, 0);
        j--;
      }
    }
  }
  return nums[0];
}

void pull_array(void* arr, int* count, int index, int type){
  // operator array
  if(type == 0){
    enum OPERATOR *op = (enum OPERATOR*)arr;
    for(int i=index; i<*count-1; i++)
      op[i] = op[i+1];
  }
  else {
    int *nums = (int*)arr;
    for(int i=index; i<*count-1; i++)
      nums[i] = nums[i+1];
  }
  (*count)--;
}

int custom_pow(int a, int b){
  int temp = 1;
  for(int i=0; i<b; i++)
    temp *= a;
  return temp;
}

void printArray(int* arr, int cnt){
  for(int i=0; i<cnt; i++)
    printf("%d ", arr[i]);
  printf("\n");
}
