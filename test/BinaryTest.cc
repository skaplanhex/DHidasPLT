#include <iostream>
#include <fstream>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <vector>
#include "EventTextToBinary.h"

using namespace std;

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

int main(int argc, char* argv[]){
	if (argc != 6){
        cout << "Usage: ./test channel ROC column row adc" << endl;
        return 0;
    }
    int channel=stoi(argv[1]);
	int ROC=stoi(argv[2]);
	int column=stoi(argv[3]);
	int row=stoi(argv[4]);
	int adc=stoi(argv[5]);
	pair<string,uint32_t> hexinfo = toHex(channel,ROC,column,row,adc);
    //cout << "now convert " << hexinfo.first << " back to hit information.." << endl;
    vector<int> res = toHitInfo(hexinfo.second);
    for (int i = 0; i < res.size(); i++){
        cout << res.at(i) << " ";
        if ( i == (res.size()-1) )
            cout << "\n";
    }
    return 0;

    
}