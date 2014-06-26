/************************************************************
**                                                         **
**       Convert PLT event data from text to binary        **
**                                                         **
**  Expects each line in the text file to be of the form:  **
**   <Channel> <ROC> <Column> <Row> <ADC> <Event Number>   **
**                                                         **
**              Written by Steven. M Kaplan                **
**  Relies heavily on bin/PLTMC.cc written by Dean Hidas   **
**                                                         **
************************************************************/

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <stdint.h>

using namespace std;

ifstream textFile;
ofstream binaryFile;

//close the file streams
void cleanUp(){
    textFile.close();
    binaryFile.close();
}
//function that actually does the work
void createBinaryFile(string textFileName, string binaryFileName){
    binaryFile.open( binaryFileName.c_str(), std::ios::out | std::ios::binary );
    textFile.open( textFileName.c_str() );
    if ( !textFile.is_open() ){
        cout << "Could not open the text file!" << endl;
        return;
    }
    string line;
    vector< vector<int> > eventInfo;
    int currentEventNumber = -10; //negative as a placeholder for the first event
    int hitCount = 0;
    bool firstLine = true;
    while( getline(textFile, line) ){
        if (line.size() == 0) continue; //skip empty lines
        int channel,ROC,column,row,eventNum,adc;
        double adcdouble;
        uint32_t n,n2;
        //create stringstream object for each line
        stringstream ss(line);
        ss >> channel >> ROC >> column >> row >> adcdouble >> eventNum;
        adc = static_cast<int>(adcdouble);
        if ( eventNum > currentEventNumber ){
            if (!firstLine){
                //stuff to write after the previous event
                if (hitCount % 2 == 0) {
                    // even number of hits.. need to fill out the word.. just print the number over two as 2x32  words
                    n  = 0xa0000000;
                    n2 = 0x00000000;
                    n  |= (hitCount / 2 + 2);
                    binaryFile.write( (char*) &n2, sizeof(uint32_t) );
                    binaryFile.write( (char*) &n, sizeof(uint32_t) );
                }
                else {
                    // Print number of hits in 1x32 bit
                    n  = 0x00000000;
                    binaryFile.write( (char*) &n, sizeof(uint32_t) );
                    binaryFile.write( (char*) &n, sizeof(uint32_t) );
                    n  = 0xa0000000;
                    n  |= (hitCount / 2 + 1);
                    binaryFile.write( (char*) &n, sizeof(uint32_t) );
                }
            }
            //stuff to begin the next event
            hitCount = 0;
            currentEventNumber = eventNum;
            n2 = (5 << 8);
            n = 0x50000000;
            n |= currentEventNumber; //i.e. the event that has just finished in the loop
            binaryFile.write( (char*) &n2, sizeof(uint32_t) );
            binaryFile.write( (char*) &n, sizeof(uint32_t) );
        }
        hitCount++;
        n = 0x00000000;
        n |= (channel << 26);
        n |= ( (ROC + 1) << 21);
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
        binaryFile.write( (char*) &n, sizeof(uint32_t) );
        ss.str( string() ); //flush stringstream for next line in the text file
        firstLine = false;
    }
    cleanUp();

}