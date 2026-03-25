#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Please provide stegano image file name in command line.\n");
        return e_failure;
    }

    // check for valid .bmp image
    if (strstr(argv[2], ".bmp") == NULL || strcmp(strstr(argv[2], ".bmp"), ".bmp") != 0)
    {
        printf("ERROR: Invalid image format. Only .bmp supported.\n");
        return e_failure;
    }
    decInfo->stego_image_fname = argv[2]; 

    // check output file name
    if (argv[3] != NULL)
    {
        int dot_count = 0;
        for (int i = 0; argv[3][i] != '\0'; i++)
        {
            if (argv[3][i] == '.')
            {
                dot_count++; 
            }
        }

        if (dot_count > 1)
        {
            printf("ERROR: Invalid output file. Multiple extensions not allowed.\n");
            return e_failure;
        }

        // separate name and extension
        char *dot = strchr(argv[3], '.');
        if (dot)
        {
            *dot = '\0'; // remove extension part
        }
        decInfo->output_fname = argv[3];
    }
    else
    {
        decInfo->output_fname = "decoded"; // default output name
    }

    return e_success;
}

Status open_files_decode(DecodeInfo *decInfo)
{
    printf("INFO: Opening required files\n");

    // open stego image in read mode
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);
        return e_failure;
    }

    printf("INFO: Opened %s\n", decInfo->stego_image_fname);
    return e_success;
}

char decode_byte_from_lsb(char *image_buffer)
{
    char data = 0;

    // read each LSB bit from 8 bytes to form a character
    for (int i = 0; i < 8; i++)
    {
        data = (data << 1) | (image_buffer[i] & 1);
    }

    return data;
}

int decode_int_from_lsb(FILE *fptr_stego_image)
{
    char arr[32];
    fread(arr, 1, 32, fptr_stego_image); // read 32 bytes from image

    int value = 0;
    // combine 32 bits to form integer
    for (int i = 0; i < 32; i++)
    {
        value = (value << 1) | (arr[i] & 1);
    }

    return value;
}

Status decode_magic_string(const char *magic_string, DecodeInfo *decInfo)
{
    char arr[8], ch;

    printf("INFO: Decoding Magic String Signature\n");
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET); // skip BMP header

    // read and compare each character of magic string
    for (int i = 0; i < strlen(magic_string); i++)
    {
        fread(arr, 1, 8, decInfo->fptr_stego_image);
        ch = decode_byte_from_lsb(arr);

        if (ch != magic_string[i])
        {
            printf("ERROR: Magic string mismatch. Invalid stego file.\n");
            return e_failure;
        }
    }

    printf("INFO: Done\n");
    return e_success;
}

int decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    int size = decode_int_from_lsb(decInfo->fptr_stego_image); // decode 32 bits for extn size
    return size;
}

Status decode_secret_file_extn(char *extn, int size, DecodeInfo *decInfo)
{
    char arr[8];

    for (int i = 0; i < size; i++)
    {
        fread(arr, 1, 8, decInfo->fptr_stego_image);
        extn[i] = decode_byte_from_lsb(arr);
    }

    extn[size] = '\0'; 

    printf("INFO: Decoding Output File Extension\n");
    printf("INFO: Done\n");

    return e_success;
}


long decode_secret_file_size(DecodeInfo *decInfo)
{
    long size = decode_int_from_lsb(decInfo->fptr_stego_image); // decode 32 bits for file size
    return size;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char arr[8];
    char ch;
    char final_output_name[50];

    // form output filename (decoded + extension)
    strcpy(final_output_name, decInfo->output_fname);
    strcat(final_output_name, decInfo->extn_secret_file);

    // open file to write decoded data
    FILE *fptr_output = fopen(final_output_name, "w");
    if (fptr_output == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open output file %s\n", final_output_name);
        return e_failure;
    }

    printf("INFO: Output File not mentioned. Creating %s as default\n", final_output_name);
    printf("INFO: Opened %s\n", final_output_name);

    // decode file data byte by byte
    for (int i = 0; i < decInfo->size_secret_file; i++)
    {
        fread(arr, 1, 8, decInfo->fptr_stego_image);
        ch = decode_byte_from_lsb(arr); 
        fputc(ch, fptr_output); 
    }

    fclose(fptr_output); 

    printf("INFO: Done\n");
    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    printf("INFO: ## Decoding Procedure Started ##\n");

    // open stego image file
    if (open_files_decode(decInfo) == e_failure)
    {
        return e_failure;
    }

    // get size for reference (not mandatory)
    get_image_size_for_bmp(decInfo->fptr_stego_image);

    // check magic string to verify image
    if (decode_magic_string("#*", decInfo) == e_failure)
    {
        return e_failure;
    }

    // decode secret file extension size and extension
    int extn_size = decode_secret_file_extn_size(decInfo);
    decode_secret_file_extn(decInfo->extn_secret_file, extn_size, decInfo);

    // decode file size
    decInfo->size_secret_file = decode_secret_file_size(decInfo);
    printf("INFO: Decoding %s File Size\n", decInfo->extn_secret_file);
    printf("INFO: Done\n");

    // decode file data
    printf("INFO: Decoding %s File Data\n", decInfo->extn_secret_file);
    decode_secret_file_data(decInfo);
    printf("INFO: Done\n");

    fclose(decInfo->fptr_stego_image); 

    printf("INFO: ## Decoding Done Successfully ##\n");
    return e_success;
}