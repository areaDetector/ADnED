
//Documentation in ADnEDFile.cpp file

#ifndef ADNEDFILE_H
#define ADNEDFILE_H

#include "epicsTypes.h"

#define ADNEDFILE_MAX_STRING 256

class ADnEDFile {

 public:
  ADnEDFile(const char *fileName);
  virtual ~ADnEDFile();
  
  void closeFile(void);
  epicsUInt32 getSize(void);
  void readDataIntoIntArray(epicsUInt32 **pArray);
  void readDataIntoDoubleArray(epicsFloat64 **pArray);
  
 private:

  //Private dynamic
  epicsUInt32 m_Size;
  FILE *p_FILE;
  char m_fileName[ADNEDFILE_MAX_STRING];

  //Private static const
  static const epicsUInt32 s_ADNEDFILE_MAX_STRING;
  static const epicsUInt32 s_ADNEDFILE_MAX_LINES;
  static const epicsUInt32 s_ADNEDFILE_STRTOL_BASE;

};

#endif //ADNEDFILE_H
