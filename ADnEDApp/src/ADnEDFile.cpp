
/**
 * Class to open and read a mapping file for ADnED.
 * The mapping files are used either for the pixel event 
 * mapping or the time-of-flight transformation (to d-space
 * for example). 
 * 
 * The file format is expected to be a ASCII
 * text, with the first line specifying the number
 * of lines in the file. Each line is will have a CR
 * and no spaces. Each line corresponds to a pixel 
 * number (with the 2nd line in the file representing
 * the first pixel ID in a detector pixel number range.
 *
 * The class provides functions to open, close and read the file
 * (converting to either ints or floats). The array
 * that is populated is expected to be managed by the 
 * calling code.
 *
 * @author Matt Pearson
 * @date Oct 2014
 */

#include "stdio.h"
//#include "dirent.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"
#include <stdexcept>

#include "ADnEDFile.h"

using std::runtime_error;

const epicsUInt32 ADnEDFile::s_ADNEDFILE_MAX_STRING = ADNEDFILE_MAX_STRING;
const epicsUInt32 ADnEDFile::s_ADNEDFILE_MAX_LINES = 1000000;
const epicsUInt32 ADnEDFile::s_ADNEDFILE_STRTOL_BASE = 10;

/**
 * Constructor. This will open the file and try to read
 * the length from the first line. If any error is encountered
 * then a std::runtime_error exception is thrown.
 */
ADnEDFile::ADnEDFile(const char *fileName) 
{
  const char *functionName = "ADnEDFile::ADnEDFile";

  m_Size = 0;
  strncpy(m_fileName, fileName, s_ADNEDFILE_MAX_STRING-1);

  printf("%s. Opening File: %s\n", functionName, m_fileName);

  if (access(m_fileName, R_OK) != 0) {
    perror(functionName);
    fprintf(stderr, "%s ERROR: File could not be read.\n", functionName);
    throw runtime_error("Failed to open file");
  }

  if ((p_FILE = fopen(m_fileName, "r")) == NULL) {
    perror(functionName);
    fprintf(stderr, "%s ERROR: File could not be opened.\n", functionName);
    throw runtime_error("Failed to open file");
  }

  char line[s_ADNEDFILE_MAX_STRING] = {0};
  char *end = NULL;

  //Get size of array (1st line in file)
  fgets(line, s_ADNEDFILE_MAX_STRING-1, p_FILE);
  long int size = strtol(line, &end, s_ADNEDFILE_STRTOL_BASE);
  m_Size = static_cast<epicsUInt32>(size);
    if ((errno != ERANGE) && (end != line)) {
    printf("%s array size: %d.\n", functionName, m_Size);
    printf("point1\n");
  } else {
    fprintf(stderr, "%s. ERROR: failed to get array size. line: %s\n", functionName, line);
    throw runtime_error("Failed to read size");
    }

  printf("%s. Lines: %d\n", functionName, m_Size);
  
}

/**
 * Destructor. This will close the file handle.
 */
ADnEDFile::~ADnEDFile()
{
  const char *functionName = "ADnEDFile::~ADnEDFile";
  
  if (p_FILE) {
    if (fclose(p_FILE)) {
      perror(functionName);
    } else {
      printf("%s. Closed File: %s\n", functionName, m_fileName);
    }
  }
  
}

/**
 * Return the size of the file read from the first line.
 * @return size
 */
epicsUInt32 ADnEDFile::getSize()
{
  return m_Size;
}

/**
 * Read the rest of file line by line. Convert each
 * number to an int, and populate array. It is expected that
 * the array has already been allocated with at least
 * a number of elements (returned by the ADnEDFile::getSize()
 * function).
 * @param Pointer to array of epicsUInt32.
 */
void ADnEDFile::readDataIntoIntArray(epicsUInt32 **pArray)
{
  const char *functionName = "ADnEDFile::readDataIntoIntArray";

  //Check array is not NULL
  if (*pArray == NULL) {
    fprintf(stderr, "%s. ERROR: Array pointer is NULL.\n", functionName);
    throw runtime_error("Array pointer is NULL");
  }
  
  const char *whitespace = "# \n\t";
  char line[s_ADNEDFILE_MAX_STRING] = {0};
  char *end = NULL;
  epicsUInt32 index = 0;

  while (fgets(line, s_ADNEDFILE_MAX_STRING-1, p_FILE)) {
    //Remove newline
    line[strlen(line)-1]='\0';
    //reject any whitespace
    if (strpbrk(line, whitespace) == NULL) {
      //printf("%s: %s\n", functionName, line);
      long int new_index = strtol(line, &end, s_ADNEDFILE_STRTOL_BASE);
      //Populate array
      if ((errno != ERANGE) && (end != line)) {
	((*pArray))[index] = static_cast<epicsUInt32>(new_index);
	++index;
      } else {
	fprintf(stderr, "%s: Stopping due to bad reading in line: %s.\n", functionName, line);
	throw runtime_error("Bad line");
      }
    } else {
      fprintf(stderr, "%s: Stopping due to whitespace in line: %s.\n", functionName, line);
      throw runtime_error("Whitespace");
    }
    memset(line, 0, sizeof(line));
  }
  
}

