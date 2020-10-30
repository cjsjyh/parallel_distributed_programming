/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include <string.h>
#include "calculator.h"

enum OPERATOR {
  PLUS = 0,
  MINUS,
  MULT,
  DIV,
  POW
};
#define MAX_PRIORITY 2
int priority[] = {2, 2, 1, 1, 0};

void string_parse(char* str, int nums[], int* num_cnt, enum OPERATOR ops[], int* op_cnt);
int calculate(int nums[], int *num_cnt, enum OPERATOR ops[], int *op_cnt);
void pull_array(void* arr, int* count, int index, int type);
int custom_pow(int, int);
void printArray(int*, int);

int *
calculate_1_svc(custom_string *argp, struct svc_req *rqstp)
{
	static int  result;
  int nums[100], num_cnt, op_cnt;
  enum OPERATOR ops[100];
	/*
	 * insert server code here
	 */
  string_parse(argp->str.str_val, nums, &num_cnt, ops, &op_cnt);
  result = calculate(nums, &num_cnt, ops, &op_cnt);

  printf("Result: %d\n", result);

	return &result;
}

void string_parse(char* str, int nums[], int* num_cnt, enum OPERATOR ops[], int* op_cnt){
  char temp[100];
  int last_op = -1;
  *num_cnt = *op_cnt = 0;

  for(int i=0; i<strlen(str); i++){
    switch (str[i]){
      case '+':
        strncpy(temp, &(str[last_op+1]), i-last_op-1);
        temp[i-last_op-1] = '\0';
        nums[(*num_cnt)++] = atoi(temp);
        ops[(*op_cnt)++] = PLUS;
        last_op = i;
        break;
      case '-':
        strncpy(temp, &(str[last_op+1]), i-last_op-1);
        temp[i-last_op-1] = '\0';
        nums[(*num_cnt)++] = atoi(temp);
        ops[(*op_cnt)++] = MINUS;
        last_op = i;
        break;
      case '/':
        strncpy(temp, &(str[last_op+1]), i-last_op-1);
        temp[i-last_op-1] = '\0';
        nums[(*num_cnt)++] = atoi(temp);
        ops[(*op_cnt)++] = DIV;
        last_op = i;
        break;
      case '*':
        strncpy(temp, &(str[last_op+1]), i-last_op-1);
        temp[i-last_op-1] = '\0';
        nums[(*num_cnt)++] = atoi(temp);
        if(i+1 < strlen(str) && str[i+1] == '*'){
          ops[(*op_cnt)++] = POW;
          i++;
        }
        else {
          ops[(*op_cnt)++] = MULT;
        }
        last_op = i;
        break;
    }
  }
  strncpy(temp, &(str[last_op+1]), strlen(str)-last_op-1);
  temp[strlen(str)-last_op-1] = '\0';
  nums[(*num_cnt)++] = atoi(temp);
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