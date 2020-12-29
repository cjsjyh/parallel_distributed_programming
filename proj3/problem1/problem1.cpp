#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#define TRUE 1
#define FALSE 0


int isPalindrome(string str){
    int left = 0;
    int right = str.length() - 1;
    while (right > left){
        if (str[left++] != str[right--])
            return FALSE;
    }
    return TRUE;
}

string reverse(string str){

}

int main(int argc, char* argv[]) {
    int count = 0;
   
    vector<string> v;
    ofstream writeFile;
    ifstream readFile;
    string nextString;

    if (argc != 4){
        printf("Invalid command line arguments!\n");
        return 0;
    }

    readFile.open("words.txt");
    writeFile.open("result.txt");
    if(!readFile.is_open()){
        printf("Read file [%s] doesn't exist!\n", argv[2]);
        return 0;
    }
    if(!writeFile.is_open()){
        printf("write file [%s] doesn't exist!\n", argv[3]);
        return 0;
    }
    
    while (readFile >> nextString)
        v.push_back(nextString);


    double start, end;
    start = omp_get_wtime();
    #pragma omp parallel for num_threads(atoi(argv[1]))
    for(int i=0; i<v.size(); i++){
        if(isPalindrome(v[i]) == TRUE){
            #pragma omp critical
            {
                writeFile << v[i] << endl;
            }
            continue;
        }
        for(int j=0; j<v.size(); j++){
            string temp = v[i];
            reverse(temp.begin(), temp.end());
            if (temp == v[j]){
                #pragma omp critical
                {
                    writeFile << v[i] << endl;
                }
                break;
            }
        }
    }
    end = omp_get_wtime();

    readFile.close();
    writeFile.close();

    printf("Work took %f seconds\n", end - start);

    return 0;
}