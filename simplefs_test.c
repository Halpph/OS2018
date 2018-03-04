#include "simplefs.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
    int i = 0;
    char entries1[8] = "11110011";
    unsigned char block1 = 0;
    char entries2[8] = "10100100";
    unsigned char block2 = 0;
    for ( i = 0; i < 8; ++i ){
        block1 |= (entries1[i] == '1') << (7 - i);
        block2 |= (entries2[i] == '1') << (7 - i);
    }
    printf("BLOCK[1]: %d\nBLOCK[2]: %d\n",block1,block2);
    BitMap* bit_map = malloc(sizeof(BitMap));
    bit_map->entries = entries1;
    bit_map->num_bits = 64;
    BitMap_print(bit_map);

    /*printf("FirstBlock size %ld\n", sizeof(FirstFileBlock));
    printf("DataBlock size %ld\n", sizeof(FileBlock));
    printf("FirstDirectoryBlock size %ld\n", sizeof(FirstDirectoryBlock));
    printf("DirectoryBlock size %ld\n", sizeof(DirectoryBlock));*/

}
