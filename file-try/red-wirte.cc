#include<unistd.h>
#include<stdio.h>
#include<iostream>
#include<cassert>
#include<cstring>
#include<fstream>
#define BUFFSIZE 1024
using namespace std;
void WriteFile(char* content, string filePath,int size){
    ofstream outFile(filePath, ios::app | ios::binary);
    outFile.write(content, size);
    outFile.close();
}
void ReadFile(const string &filePath){
    ifstream inFile(filePath, ios::in | ios::binary);
    if (!inFile) {
        cout << "error" << endl;
        return ;
    }
    char c[BUFFSIZE];
    //连续以行为单位，读取 in.txt 文件中的数据
//    while (inFile.getline(c,BUFFSIZE)) {
//        WriteFile(c,"./receved-file");
//    }
    while(inFile.read(c,BUFFSIZE)){
        WriteFile(c,"./receved-file.h5",BUFFSIZE);
    }  
    int size = inFile.gcount();
    WriteFile(c,"./receved-file.h5",size);
    inFile.close();
}
int main(){
    char c[BUFFSIZE];
    ReadFile("./4LayerModel.h5");
    return 0;

}
