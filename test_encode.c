#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "decode.h"
#include "types.h"
#include "common.h"

int main(int argc, char *argv[])
{
    // check if user has given required arguments
    if (argc < 2)
    {
        printf("ERROR: Insufficient arguments!\n");
        printf("Usage for Encoding: ./a.out -e <input.bmp> <secret.txt> <newimg.bmp>\n");
        printf("Usage for Decoding: ./a.out -d <newimg.bmp> <newimg.txt>\n");
        return 1;
    }

    // check if operation is encoding
    if (strcmp(argv[1], "-e") == 0)
    {
        EncodeInfo encInfo; // structure to hold encoding information

        printf("INFO: Starting Encoding Process...\n");

        // step 1: validate input and output files
        if (read_and_validate_encode_args(argv, &encInfo) == e_failure)
        {
            printf("ERROR: read_and_validate_encode_args failed\n");
            return 1;
        }

        // step 2: perform encoding process
        if (do_encoding(&encInfo) == e_failure)
        {
            printf("ERROR: Encoding failed!\n");
            return 1;
        }

        printf("INFO: Encoding done successfully!\n");
    }

    // check if operation is decoding
    else if (strcmp(argv[1], "-d") == 0)
    {
        DecodeInfo decInfo; // structure to hold decoding information

        printf("INFO: Starting Decoding Process...\n");

        // step 1: validate decoding arguments
        if (read_and_validate_decode_args(argv, &decInfo) == e_failure)
        {
            printf("ERROR: read_and_validate_decode_args failed\n");
            return 1;
        }

        // step 2: perform decoding process
        if (do_decoding(&decInfo) == e_failure)
        {
            printf("ERROR: Decoding failed!\n");
            return 1;
        }

        printf("INFO: Decoding done successfully!\n");
    }

    // invalid input (not -e or -d)
    else
    {
        printf("ERROR: Invalid operation type!\n");
        printf("Use -e for encoding or -d for decoding.\n");
        return 1;
    }

    return 0; // program ends
}
