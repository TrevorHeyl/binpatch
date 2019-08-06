/*

MIT License:

Copyright (c) 2019 Trevor Heyl

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


Description:

Binary patch utility V 0.98, by Trevor Heyl, is a command line utility written in C++ to permit build tools to modify the content of binary files.
This useful for pust build operations to patchin serial numbers, configuration items and other binary data that is unique to a build.
This utility has various patch modes, some examples are listed below to illustrate:

Scenario 1 -  Patch in a binary number at an address offset
-----------------------------------------------------------
    Assume you have a binary file called build.bin and at address offset 0x100 you want to add a version number of 0x0201 and you want the output file named
    build_2.1.bin.Note that the binary number is limited to 8 bytes and the write order is big endian ( MS byte is at first or lowest address)

    patchbin -ibuild.bin -obuild_2.1.bin -a0x100 -B0x0201

    before:
    0x000100 00 00 00 00 00 00
    after:
    0x000100 02 01 00 00 00 00

Scenario 2 -  Patch in a Text string at an address offset
-----------------------------------------------------------
    Assume you have a binary file called build.bin and at address offset 0x100 you want to add a user name TREVOR and you want the output file named
    build_.bin

    patchbin -ibuild.bin -obuild_.bin -a0x100 -TTREVOR

    before:
    0x000100 00 00 00 00 00 00 00 00 00
    after:
    0x000100 54 52 45 56 4F 52 00 00 00  "TREVOR"


Scenario 3 -  Patch in a Text string  after a TEXT marker in the binary
-----------------------------------------------------------------------
    Assume you have a binary file called build.bin and somewher ein that file is the Text string USERNAME: and you need
    add a username after that.

    patchbin -ibuild.bin -obuild_named.bin -tUSERNAME: -T"TREVOR    "

Scenario 4 -  Patch in a Text string  over a TEXT marker in the binary
-----------------------------------------------------------------------
    Similar to scenario 3, but Text string USERNAME: replaced with your new username, specify the -z option

    patchbin -ibuild.bin -obuild_named.bin -tUSERNAME: -T"TREVOR   " -z

Scenario 5 -  Patch in a Text string after a binary marker in the binary
-----------------------------------------------------------------------
    Assume you have a binary file called build.bin and somewhere in that file is the binary pattern 0x55AA55AA55 and you need
    add a test string after that.

    patchbin -ibuild.bin -obuild_.bin -b0x55AA55AA55 -T"TREVOR    "

    before:
    0x00210B 00 55 AA 55 AA 55 AA 55 00 00 00 00 00 00 00 00 00 00 00
    after:
    0x00210B 00 55 AA 55 AA 55 AA 55 54 52 45 56 4F 52 20 20 20 20 00


*/

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <bits/stdc++.h>
#include <string>
#include <cstring>
#include <climits>

using namespace std;

const char PATCH_VERSION[] = "0.98";

/*
Copy_File
    Binary copy the contents of one file to another
*/
unsigned int Copy_File(fstream &fin, ofstream &fout) {

      unsigned int fsize = 0 ;
      char buf[1024];

      do {
                  fin.read(&buf[0], 1024);      // Read at most n bytes into
                  fout.write(&buf[0], fin.gcount()); // buf, then write the buf to
                  fsize += fin.gcount();
      } while (fin.gcount() > 0);
      return fsize;
}

/*
Populate_Patch_Data
    Build the file write buffer that will be used to patch over the old data
    buf : buffer to build
    binarypattern :  64 bit value that hold the binary patch data,
        This data will be read out LSB first, thus the funtion is 'length aware'
    patchen : number of bytes to patch
*/
void   Populate_Patch_Data( char * buf, uint64_t binarypattern , int patchlen)  {

    int c;
    char *pget = (char *)&binarypattern;

    for(c=0; c< patchlen; c++) {
            buf[c] = pget[patchlen-c-1];
    }
}



/*
Clean_Hex_String
    Remove leading 0X if present , consider lower and upper case as well
*/
void Clean_Hex_String( string &s ) {

     size_t n=0;
     size_t N=0;
     n = s.find('x');
     N = s.find('X');
     if(n>=0 && n<128)   s = s.substr(n+1,string::npos);
     else if (N>=0 && N < 128 ) s = s.substr(N+1,string::npos);
}


/*
Parse_ASCIIHex_Param_To_Int
    Convert a HEX string in ASCII format to binary,pack into a 64 bit unsigned int
    sin : String to convert, eg. 0X1234 0x1234567890ABCDEF
    Returns : uint64 of sin
*/
uint64_t Parse_ASCIIHex_Param_To_Int( string sin ) {

        uint64_t int_out;
        stringstream ss;
        ss << std::hex << sin;
        ss >> int_out;
        return int_out;
}

/*
DoHelp
    Version and Usage information
*/
void  DoHelp(void) {

    cout << "Binary patch utility Version " <<  PATCH_VERSION  << " by Trevor Heyl" << endl;
    cout << "Usage :" << endl;
    cout << "-i input file" << endl;
    cout << "-o output file" << endl;
    cout << "Only one of the next 3" << endl;
    cout << "-a address to start patch in HEX 0x0 to 0xFFFFFFFF" << endl;
    cout << "-t text pattern to find after which to patch, max 48 characters " << endl;
    cout << "-b binary(hex ASCII ) pattern to find after which to patch,max 8 bytes " << endl;
    cout << "Either one or the other of the next 2,not both" << endl;
    cout << "-B patch value in binary (hex ASCII), maximum 8 bytes eg: -B0x1234" << endl;
    cout << "-T patch value in test, eg -TVERSION1.0, max 48 characters" << endl;
}


/*
Find_Pattern_In_File
    Search a file in bonary mode, find the matching pattern and return the offset index into the file where the
    pattern is found. th eoffset can be modifed according to the z-flag definition
*/
uint32_t Find_Pattern_In_File(string filename,char * pattern, int siz, bool z) {

    fstream fin;
	ifstream::pos_type size;
	fin.open( filename, ios::in|ios::binary|ios::ate );
	char* fData;

	size = fin.tellg();
	fin.seekg(0, ios::beg);

	fData = new char[ size ];
	fin.read( fData, size );

	//search
	int pdata_offs=0;
	for ( int i = 0; i < size; i++ )
	{
		if (*fData == *(pattern+pdata_offs) ) {
            pdata_offs++;
		} else {
		    pdata_offs = 0;
		}
		if ( pdata_offs== siz) {
            // found
            if (!z) return i+1; // start after pattern - defualt
            else return i-siz+1; // start where pattern started
		}
		fData++;
	}

    fin.close();
    delete[] fData;

    return (ULONG_MAX); // not found

}


/*

Returns 0 for success or -1 for fail
*/
int main(int argc, char *argv[])
{
    int opt;
    int searchpatterncnt=0;
    int patchpatterncnt=0;
    uint32_t startaddr;
    uint64_t binarypattern;
    bool patch_pos = 0;

    string infilename ="";
    string outfilename ="";
    string patchstartaddr ="";
    string binsearchpattern ="";
    string textsearchpattern ="";
    string binarypatchpattern ="";
    string textpatchpattern ="";
    /////////////////////////////////////////////////////////
    // Get the command line arguments
    /////////////////////////////////////////////////////////
    while((opt = getopt(argc, argv, ":i:o:a:t:b:B:T:Hh?z" )) != -1)
    {

/*         -i input file
           -o output file
           // Either on or the other of the next 3,not both
           -a address to start patch
           -t text pattern to find after which to patch
           -b binary(hex ASCII ) pattern to find after which to patch
           // Either on or the other of the next 2,not both
           -B patch value in binary (hex ASCII)
           -T patch value in test
           -z For text and binary search patterns specify if the patch address is after the pattern or at the start of the pattern - -z means at start of pattern
*/
        switch(opt)
        {
            case '?':
            case 'h':
            case 'H':
                DoHelp();
                return 0 ;
                break;
            case 'i':
                cout << "Input file: " <<  optarg << endl;
                infilename = optarg;
                break;
            case 'o':
                cout << "Output file: " <<  optarg << endl;
                outfilename = optarg;
                break;
            case 'z':
                patch_pos = 1;
                break;
            case 'a':
                //cout << "Patch start address: " <<  optarg << endl;
                patchstartaddr = optarg;
                searchpatterncnt++;
                break;
            case 't':
                //cout << "Text pattern to find: " <<  optarg << endl;
                textsearchpattern = optarg;
                searchpatterncnt++;
                break;
            case 'b':
                //cout << "Binary pattern to find: " <<  optarg << endl;
                binsearchpattern = optarg;
                searchpatterncnt++;
                break;
            case 'B':
                //cout << "Binary pattern to patch into file " <<  optarg << endl;
                binarypatchpattern = optarg;
                patchpatterncnt++;
                break;
            case 'T':
                //cout << "Text pattern to patch into file " <<  optarg << endl;
                textpatchpattern = optarg;
                patchpatterncnt++;
                break;
            default:
                cout << "Unknown paramter: " << opt << endl;

        }
    }

    /////////////////////////////////////////////////////////
    // optind is for the extra arguments
    // which are not parsed
    /////////////////////////////////////////////////////////
    for(; optind < argc; optind++){
        printf("Unrecognised arguments: %s\n", argv[optind]);
    }

    /////////////////////////////////////////////////////////
    // Checks for correct minimal required arguments
    /////////////////////////////////////////////////////////
    if ( !infilename.length()  )  {  cout << endl << "Please specify the input filename with -i";  return -1;}
    if ( !outfilename.length()  ) {  cout << endl << "Please specify the output filename with -o"; return -1; }
    if ( patchpatterncnt == 0)    {  cout << endl << "Pleas especify one patch pattern with -B or-T"; return -1; }
    if ( patchpatterncnt > 1 )    {  cout << endl << "Too many patch pattern specifiers, choose only one of -B or-T"; return -1; }
    if ( searchpatterncnt == 0)   {  cout << endl << "Please specify one search  pattern with -a, -t  or -b"; return -1; }
    if ( searchpatterncnt > 1 )   {  cout << endl << "Too many search pattern specifiers, choose only one of  -a, -t  or -b"; return -1; }


    /////////////////////////////////////////////////////////
    // First get the patch start address, this may be from  a supplied address (-a), a supplied binar pattern (-b)
    //   or a supplied text pattern (-t).  The patch is done by defualt at the start address or at the address
    //   immediately after the match pattern. Optional parameter -z places the patch address at the start location
    //   of a matched pattern
    /////////////////////////////////////////////////////////

    /////////////////////////////////////////////////////////
    // Check if we have a patch address (-a) option
    /////////////////////////////////////////////////////////
    if ( patchstartaddr.length() > 0 ) {
        Clean_Hex_String(patchstartaddr);
        startaddr = Parse_ASCIIHex_Param_To_Int(patchstartaddr);
    }
   /////////////////////////////////////////////////////////
   // Check if we have a text search pattern (-t) option
   /////////////////////////////////////////////////////////
   else if ( textsearchpattern.length() > 0 ) {
        // user provide patch text pattern, so we will patch after this pattern in infile or at
        //  the start of this opattern if -z is specified
        char* pData;
        int siz = textsearchpattern.length();
        pData = new char[ siz+1 ];
        strcpy(pData,textsearchpattern.c_str());

        startaddr =   Find_Pattern_In_File(infilename ,pData ,siz, patch_pos );
        if (startaddr == ULONG_MAX) {
            cout << endl << "Pattern not found, exiting!";
            delete[] pData;
            return -1;
        }

        delete[] pData;

    /////////////////////////////////////////////////////////
   // or check if we have a binary search pattern (-b) option
   /////////////////////////////////////////////////////////
   } else if (binsearchpattern.length() > 0)   {

        Clean_Hex_String(binsearchpattern);
        uint64_t pattern = Parse_ASCIIHex_Param_To_Int(binsearchpattern);
        int siz = binsearchpattern.length()/2 + binsearchpattern.length() % 2;

        char pData[9];

        Populate_Patch_Data( pData, pattern ,siz) ;
        startaddr =   Find_Pattern_In_File(infilename ,pData ,siz, patch_pos );
        if (startaddr == ULONG_MAX) {
            cout << endl << "Pattern not found, exiting!";
            return -1;
        }

    } else {
        cout << endl << "Invalid patch start address, exiting!";
    }


    /////////////////////////////////////////////////////////
    // We now have the start patch address, lets patch either binary or Text data
    /////////////////////////////////////////////////////////

      ///////////////////////////////////////////////////////////////////
      // CASE 0 : user provided patch start address, and text patch data
      ///////////////////////////////////////////////////////////////////
        if( textpatchpattern.length() > 0) {
            int patchlen = textpatchpattern.length();
            if (patchlen < 1) {
                cout << endl << "Nothing to patch - exiting.";
                return -1;
            }
            if (patchlen > 48) {
                cout << endl << "Patch text data too large, must be 48 characters or less";
                return -1;
            }
            cout << "Patching " << textpatchpattern << " of length " << patchlen << " byte(s), to address 0X" << std::hex << startaddr  ;
            // we can patch a maximum of 8 bytes, check size
            fstream fin;
            fin.open(infilename,ios::in | ios::binary );
            if(!fin) {
                cout << endl << "Input file " << infilename << "  not found, exiting.";
                return -1;
            }
            ofstream fout;
            fout.open(outfilename, ios::out | ios::binary);


            if ( Copy_File( fin, fout) < (startaddr + patchlen)) {
                cout << endl << "Patch data is outside file, exiting." ;
                fin.close();
                fout.close();
                return -1;
            }

            char buf[50];
            // populate patch data
            //memcpy(buf,(char *)textpatchpattern,patchlen);
            strcpy(buf,textpatchpattern.c_str());

            fout.seekp(startaddr);
            fout.write(buf,patchlen);
            fin.close();
            fout.close();
            cout << endl << "Success!";

        } else
        ///////////////////////////////////////////////////////////////////
        // CASE 1 : user provided patch start address, and binary patch data
        ///////////////////////////////////////////////////////////////////
        if (binarypatchpattern.length() > 0 ) {
            Clean_Hex_String(binarypatchpattern);
            binarypattern = Parse_ASCIIHex_Param_To_Int(binarypatchpattern);
            int patchlen = binarypatchpattern.length()/2 + binarypatchpattern.length() % 2;
            if (patchlen < 1) {
                cout << endl << "Nothing to patch - exiting.";
                return -1;
            }
            cout << "Patching " << binarypatchpattern << " of length " << patchlen << " byte(s), to address 0X" << std::hex << startaddr  ;
            // we can patch a maximum of 8 bytes, check size
            fstream fin;
            fin.open(infilename,ios::in | ios::binary );
            if(!fin) {
                cout << endl << "Input file " << infilename << "  not found, exiting.";
                return -1;
            }
            ofstream fout;
            fout.open(outfilename, ios::out | ios::binary);


            if ( Copy_File( fin, fout) < (startaddr + patchlen)) {
                cout << endl << "Patch data is outside file, exiting." ;
                fin.close();
                fout.close();
                return -1;
            }

            char buf[12];
            // populate patch data
            Populate_Patch_Data( buf, binarypattern ,patchlen) ;

            fout.seekp(startaddr);
            fout.write(buf,patchlen);
            fin.close();
            fout.close();
            cout << endl << "Success!";

        } else {
            cout << "No patch pattern specified - exiting without change" << endl;
            return -1;
        }

    return 0;
}
