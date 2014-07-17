////////////////////////////////////////////////////////////////////
//
// Dean Andrew Hidas <Dean.Andrew.Hidas@cern.ch>
//
// Created on: Tue Jul  1 18:44:59 CEST 2014
//
////////////////////////////////////////////////////////////////////


#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <stdint.h>

#include "PLTHit.h"

int DeanTextToBinary (std::string const InFileName, std::string const OutFileName)
{
  //check that all the filestreams are operational
  std::ifstream InFile(InFileName.c_str());
  if (!InFile.is_open()) {
    std::cerr << "ERROR: cannot open input file: " << InFileName << std::endl;
    throw;
  }

  std::ofstream fout(OutFileName.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    std::cerr << "ERROR: cannot open output file" << std::endl;
    throw;
  }

  uint32_t n, n2; //the usual bit holders

  int Channel, ROC, Column, Row, ADC, EventNumber; //store the hit info
  int LastEventNumber = -1;
  bool IsFirstEvent = true;
  std::vector<PLTHit> Hits; //vector of hits filled for each event most likely

  //loop goes until the ifstream can't read the info anymore (i.e. reached the end of the file)
  for ( ; InFile >> Channel >> ROC >> Column >> Row >> ADC >> EventNumber; ) {
    if (IsFirstEvent) { //set stuff for first event (not necessarily event 1)
      LastEventNumber = EventNumber;
      IsFirstEvent = false;
    }
    if (EventNumber != LastEventNumber) { //if we get to a line that has a new event

      // Delete all hits and clear vector
      n2 = (5 << 8);
      n =  0x50000000;
      n |= LastEventNumber;
      fout.write( (char*) &n2, sizeof(uint32_t) );
      fout.write( (char*) &n, sizeof(uint32_t) );


      for (size_t ihit = 0; ihit != Hits.size(); ++ihit) {
        n = 0x00000000;
        PLTHit* Hit = &Hits[ihit];
        //printf("Channel ROC Row Col ADC: %2i %1i %2i %2i %4i %12i\n", Hit->Channel(), Hit->ROC(), Hit->Row(), Hit->Column(), Hit->ADC(), ievent);
        //fprintf(ff, "%2i %1i %2i %2i %4i %i\n", Hit->Channel(), Hit->ROC(), Hit->Column(), Hit->Row(), Hit->ADC(), EventNumber);
        printf("%2i %1i %2i %2i %4i %i\n", Hit->Channel(), Hit->ROC(), Hit->Column(), Hit->Row(), Hit->ADC(), EventNumber);

        n |= (Hit->Channel() << 26);
        n |= ( (Hit->ROC() + 1) << 21);

        if (Hit->Column() % 2 == 0) {
          n |= ( ((80 - Hit->Row()) * 2) << 8 );
        } else {
          // checked, correct
          n |= ( ((80 - Hit->Row()) * 2 + 1) << 8 );
        }

        if (Hit->Column() % 2 == 0) {
          n |= ( ((Hit->Column()) / 2) << 16  );
        } else {
          n |= (( (Hit->Column() - 1) / 2) << 16  );
        }
        n |= Hit->ADC();


        //if (Hit->ROC() == 2) {
        //  printf("WORD: %X\n", (n &  0x3e00000));
        //}

        fout.write( (char*) &n, sizeof(uint32_t) );
      }

      if (Hits.size() % 2 == 0) {
        // even number of hits.. need to fill out the word.. just print the number over two as 2x32  words
        n  = 0xa0000000;
        n2 = 0x00000000;
        n  |= (Hits.size() / 2 + 2);
        fout.write( (char*) &n2, sizeof(uint32_t) );
        fout.write( (char*) &n, sizeof(uint32_t) );
      } else {
        // Print number of hits in 1x32 bit
        n  = 0x00000000;
        fout.write( (char*) &n, sizeof(uint32_t) );
        fout.write( (char*) &n, sizeof(uint32_t) );
        n  = 0xa0000000;
        n  |= (Hits.size() / 2 + 1);
        fout.write( (char*) &n, sizeof(uint32_t) );
      }




      LastEventNumber = EventNumber;
      Hits.clear();
    }

    Hits.push_back( PLTHit(Channel, ROC, Column, Row, ADC) );


  }



      // Delete all hits and clear vector
      n2 = (5 << 8);
      n =  0x50000000;
      n |= LastEventNumber;
      fout.write( (char*) &n2, sizeof(uint32_t) );
      fout.write( (char*) &n, sizeof(uint32_t) );


      for (size_t ihit = 0; ihit != Hits.size(); ++ihit) {
        n = 0x00000000;
        PLTHit* Hit = &Hits[ihit];
        //printf("Channel ROC Row Col ADC: %2i %1i %2i %2i %4i %12i\n", Hit->Channel(), Hit->ROC(), Hit->Row(), Hit->Column(), Hit->ADC(), ievent);
        //fprintf(ff, "%2i %1i %2i %2i %4i %i\n", Hit->Channel(), Hit->ROC(), Hit->Column(), Hit->Row(), Hit->ADC(), EventNumber);
        printf("%2i %1i %2i %2i %4i %i\n", Hit->Channel(), Hit->ROC(), Hit->Column(), Hit->Row(), Hit->ADC(), EventNumber);

        n |= (Hit->Channel() << 26);
        n |= ( (Hit->ROC() + 1) << 21);

        if (Hit->Column() % 2 == 0) {
          n |= ( ((80 - Hit->Row()) * 2) << 8 );
        } else {
          // checked, correct
          n |= ( ((80 - Hit->Row()) * 2 + 1) << 8 );
        }

        if (Hit->Column() % 2 == 0) {
          n |= ( ((Hit->Column()) / 2) << 16  );
        } else {
          n |= (( (Hit->Column() - 1) / 2) << 16  );
        }
        n |= Hit->ADC();


        //if (Hit->ROC() == 2) {
        //  printf("WORD: %X\n", (n &  0x3e00000));
        //}

        fout.write( (char*) &n, sizeof(uint32_t) );
      }

      if (Hits.size() % 2 == 0) {
        // even number of hits.. need to fill out the word.. just print the number over two as 2x32  words
        n  = 0xa0000000;
        n2 = 0x00000000;
        n  |= (Hits.size() / 2 + 2);
        fout.write( (char*) &n2, sizeof(uint32_t) );
        fout.write( (char*) &n, sizeof(uint32_t) );
      } else {
        // Print number of hits in 1x32 bit
        n  = 0x00000000;
        fout.write( (char*) &n, sizeof(uint32_t) );
        fout.write( (char*) &n, sizeof(uint32_t) );
        n  = 0xa0000000;
        n  |= (Hits.size() / 2 + 1);
        fout.write( (char*) &n, sizeof(uint32_t) );
      }






  fout.close();



  return 0;
}


int main (int argc, char* argv[])
{
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " [InFileName] [OutFileName]" << std::endl;
    return 1;
  }

  std::string const InFileName  = argv[1];
  std::string const OutFileName = argv[2];

  DeanTextToBinary(InFileName, OutFileName);

  return 0;
}
