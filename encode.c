#include <stdio.h>
#include "encode.h"
#include "types.h"
#include <string.h>
#include "common.h"

uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    fseek(fptr_image, 18, SEEK_SET);
    fread(&width, sizeof(int), 1, fptr_image); 
    fread(&height, sizeof(int), 1, fptr_image); 

    printf("width = %u\n", width);
    printf("height = %u\n", height);

    return width * height * 3; 
}

Status open_files(EncodeInfo *encInfo)
{
    printf("INFO: Opening required files\n");

    // open source image part
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "rb");
    if (encInfo->fptr_src_image == NULL)
    {
        printf("ERROR: Unable to open file %s\n", encInfo->src_image_fname);
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->src_image_fname);

    // open secret file part
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "rb");
    if (encInfo->fptr_secret == NULL)
    {
        printf("ERROR: Unable to open file %s\n", encInfo->secret_fname);
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->secret_fname);

    // open output stego image part
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "wb");
    if (encInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Unable to open file %s\n", encInfo->stego_image_fname);
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->stego_image_fname);
    printf("INFO: Done\n\n");

    return e_success;
}

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (argv[2] == NULL)
    {
        printf("ERROR: Source image file not mentioned\n");
        return e_failure;
    }

    // check if image ends with .bmp or not
    if (strstr(argv[2], ".bmp") == NULL || strcmp(strstr(argv[2], ".bmp"), ".bmp") != 0)
    {
        printf("ERROR: Invalid source image file. Must end with .bmp\n");
        return e_failure;
    }
    encInfo->src_image_fname = argv[2];

    if (argv[3] == NULL)
    {
        printf("ERROR: Secret file not mentioned\n");
        return e_failure;
    }

    // check for valid secret file extension or not
    if (!(
        (strstr(argv[3], ".txt") && strcmp(strstr(argv[3], ".txt"), ".txt") == 0) ||
        (strstr(argv[3], ".c") && strcmp(strstr(argv[3], ".c"), ".c") == 0) ||
        (strstr(argv[3], ".sh") && strcmp(strstr(argv[3], ".sh"), ".sh") == 0) ||
        (strstr(argv[3], ".h") && strcmp(strstr(argv[3], ".h"), ".h") == 0)))
    {
        printf("ERROR: Secret file must end with .txt, .c, .sh, or .h\n");
        return e_failure;
    }
    encInfo->secret_fname = argv[3];

    // check output image name, if not given create default or not
    if (argv[4] == NULL)
    {
        printf("INFO: Output File not mentioned. Creating steged_img.bmp as default\n");
        encInfo->stego_image_fname = "steged_img.bmp";
    }
    else
    {
        if (strstr(argv[4], ".bmp") == NULL || strcmp(strstr(argv[4], ".bmp"), ".bmp") != 0)
        {
            printf("ERROR: Invalid output file. Must end with .bmp\n");
            return e_failure;
        }
        encInfo->stego_image_fname = argv[4];
    }

    return e_success;
}

Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image); 

    long total_bits_needed = (2 + 4 + 4 + 25) * 8 + 54; 

    if (encInfo->image_capacity > total_bits_needed)
    {
        return e_success;
    }
    else
    {
        return e_failure;
    }
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];

    rewind(fptr_src_image); 
    fread(header, 1, 54, fptr_src_image); 
    fwrite(header, 1, 54, fptr_dest_image); 

    return e_success;
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 7; i >= 0; i--)
    {
        char bit = (data >> i) & 1; 
        image_buffer[7 - i] = (image_buffer[7 - i] & (~1)) | bit; 
    }

    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    char arr[8];

    for (int i = 0; i < strlen(magic_string); i++)
    {
        fread(arr, 1, 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(magic_string[i], arr);
        fwrite(arr, 1, 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    char arr[32];

    fread(arr, 1, 32, encInfo->fptr_src_image);
    for (int i = 31; i >= 0; i--)
    {
        arr[31 - i] = (arr[31 - i] & (~1)) | ((size >> i) & 1); 
    }
    fwrite(arr, 1, 32, encInfo->fptr_stego_image);

    return e_success;
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    char arr[8];

    // encode each character of file extension
    for (int i = 0; i < strlen(file_extn); i++)
    {
        fread(arr, 1, 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(file_extn[i], arr);
        fwrite(arr, 1, 8, encInfo->fptr_stego_image);
    }

    return e_success;
}


Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char arr[32];

    fread(arr, 1, 32, encInfo->fptr_src_image);
    for (int i = 31; i >= 0; i--)
    {
        arr[31 - i] = (arr[31 - i] & (~1)) | ((file_size >> i) & 1); // write each bit of size
    }
    fwrite(arr, 1, 32, encInfo->fptr_stego_image);

    return e_success;
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    rewind(encInfo->fptr_secret); // go to start of secret file
    char arr[8];

    // read one byte from secret file at a time
    for (int i = 0; i < encInfo->size_secret_file; i++)
    {
        fread(encInfo->secret_data, 1, 1, encInfo->fptr_secret);
        fread(arr, 1, 8, encInfo->fptr_src_image);
        encode_byte_to_lsb(encInfo->secret_data[0], arr);
        fwrite(arr, 1, 8, encInfo->fptr_stego_image);
    }

    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char buffer[1024];
    size_t bytes;

    // copy all remaining bytes in chunks
    while ((bytes = fread(buffer, 1, sizeof(buffer), fptr_src)) > 0)
    {
        fwrite(buffer, 1, bytes, fptr_dest);
    }

    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    printf("INFO: ## Encoding Procedure Started ##\n");

    if (open_files(encInfo) == e_failure)
    {
        return e_failure;
    }

    // calculate secret file size
    fseek(encInfo->fptr_secret, 0, SEEK_END);
    encInfo->size_secret_file = ftell(encInfo->fptr_secret);
    rewind(encInfo->fptr_secret);

    printf("INFO: Checking %s file size\n", encInfo->secret_fname);
    printf("INFO: Done. File not empty\n");

    // check if image has enough capacity
    printf("INFO: Checking if %s has enough space for %s\n", encInfo->src_image_fname, encInfo->secret_fname);
    if (check_capacity(encInfo) == e_failure)
    {
        printf("ERROR: Image capacity insufficient\n");
        return e_failure;
    }
    printf("INFO: Done. Capacity OK\n");

    // copy BMP header before encoding starts
    printf("INFO: Copying BMP Header\n");
    copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    printf("INFO: Header Copied\n");

    // encode magic string
    printf("INFO: Encoding Magic String\n");
    encode_magic_string("#*", encInfo);
    printf("INFO: Magic String Encoded\n");

    // encode file extension
    printf("INFO: Encoding Secret File Extension\n");
    char *extn = strrchr(encInfo->secret_fname, '.');
    if (extn == NULL)
    {
        printf("ERROR: Secret file has no extension.\n");
        return e_failure;
    }

    encode_secret_file_extn_size(strlen(extn), encInfo);
    encode_secret_file_extn(extn, encInfo);
    printf("INFO: Extension Encoded\n");

    // encode file size
    printf("INFO: Encoding Secret File Size\n");
    encode_secret_file_size(encInfo->size_secret_file, encInfo);
    printf("INFO: Size Encoded\n");

    // encode file data
    printf("INFO: Encoding Secret File Data\n");
    encode_secret_file_data(encInfo);
    printf("INFO: Data Encoded\n");

    // copy rest of the image data as it is
    printf("INFO: Copying Remaining Image Data\n");
    copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image);
    printf("INFO: Remaining Data Copied\n");

    // close all files
    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    printf("INFO: ## Encoding Done Successfully ##\n");

    return e_success;
}