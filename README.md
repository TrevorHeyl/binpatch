Description:

Binary patch utility V 0.98, by Trevor Heyl, is a command line utility written in C++ to permit build tools to modify the content of binary files. Note the LICENSE.txt file which is the MIT license

This project was developed with CodeBlocks V 17.2 under Windows 8 but should compile under and C++11 and GNU compiler.

This utility is useful for post build operations to patchin serial numbers, configuration items and other binary data that is unique to a build.
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

All command line options:
-------------------------
    Files:
        -i input file
        -o output file
    Only one of the next 3
        -a address to start patch in HEX 0x0 to 0xFFFFFFFF (0x can be ommitted)
        -t text pattern to find after which to patch, max 48 characters 
        -b binary(hex ASCII ) pattern to find after which to patch, max 8 bytes eg,-b0x1234
    Either one or the other of the next 2,not both
        -B patch value in binary (hex ASCII), maximum 8 bytes eg: -B0x1234
        -T patch value in test, eg -TVERSION1.0, max 48 characters

