#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include "EventTextToBinary.h"

using namespace std;

ofstream newTextFile;

pair<string,uint32_t> toHex(int channel, int ROC, int column, int row, int adc){
    uint32_t n = 0x00000000;
    n |= (channel << 26);
    n |= ( (ROC) << 21); //was ROC+1 because output of FED is 1,2,3
    if (column % 2 == 0) {
        n |= ( ((80 - row) * 2) << 8 );
    }
    else {
        n |= ( ((80 - row) * 2 + 1) << 8 );
    }
    if (column % 2 == 0) {
        n |= ( (column / 2) << 16  );
    } 
    else {
        n |= (( (column - 1) / 2) << 16  );
    }
    //casting to force adc to be an int, is this ok? It needs to be in order to do binary operations!
    n |= adc;
    stringstream ss;
    ss << std::hex << n;
    string res = ss.str();
    //cout << res << endl;
    pair<string,uint32_t> p(res,n);
    return p;

}
int convPXL(int IN){
    if (IN % 2 == 0){
        return (((80-IN)*2) << 8);
    }
    else{
        return (((80-IN)*2+1) << 8);
    }
  //return IN % 2 == 1 ? 80 - (IN - 1) / 2 : (IN) / 2 - 80;
}
vector<int> toHitInfo(uint32_t word){
    const uint32_t plsmsk = 0xff;
    const uint32_t pxlmsk = 0xff00;
    const uint32_t dclmsk = 0x1f0000;
    const uint32_t rocmsk = 0x3e00000;
    const uint32_t chnlmsk = 0xfc000000;
    uint32_t chan = ((word & chnlmsk) >> 26);
    uint32_t roc  = ((word & rocmsk)  >> 21);
    int mycol = 0;
    if (convPXL((word & pxlmsk) >> 8) <= 0) { //changed > to <=
    // Odd
    mycol = ((word & dclmsk) >> 16) * 2 + 1;
    //std::cout << "MYCOL A: " << mycol << std::endl;
    } else {
    // Even
    mycol = ((word & dclmsk) >> 16) * 2;
    //std::cout << "MYCOL B: " << mycol << std::endl;
    }
    int row = abs(convPXL((word & pxlmsk) >> 8));
    int adc = word & plsmsk; //ADC MUST BE BETWEEN 0-255!!!
    vector<int> v;
    v.push_back(chan);
    v.push_back(roc);
    v.push_back(mycol);
    v.push_back(row);
    v.push_back(adc);
    return v;
}
string toString(char c){
    stringstream ss;
    ss << c;
    string res = ss.str();
    return res;
}
// first convert text to binary, then convert it back with own code to debug the conversion
int main(int argc, char* argv[]){
    if (argc != 3){
        cout << "Usage: ./BinaryTest textFileName binaryFileName" << endl;
        return 0;
    }
    string textFileName = argv[1];
    string binaryFileName = argv[2];
    //plan: use eventtexttobinary to convert text file to binary, implement a readback script of my writing, figure out what's going on
    createBinaryFile(textFileName,binaryFileName);
    int textFileNameSize = textFileName.size();
    string newTextFileName = "";
    for (int i = 0; i<textFileNameSize; i++){
        string c = toString(textFileName.at(i));
        newTextFileName.append(c);
        if (i == textFileNameSize-1) break; //to not get an out of range issue with the next line
        if (toString(textFileName.at(i+1)) == ".") newTextFileName.append("TWO");
    }
    newTextFile.open( newTextFileName.c_str() );

    return 0;
}