#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const unsigned char HEADER[] = {0xFF, 0xD8, 0xFF};  // JPEG start
const unsigned char FOOTER[] = {0xFF, 0xD9};        // JPEG end

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: (sudo) %s <input_device_path> <output_directory>\n", argv[0]);
        return 1;
    }
    FILE* input_file = fopen(argv[1], "rb");
    if (!input_file)
    {
        perror("Error opening input device");
        return 1;
    }

    unsigned char* buffer = malloc(1024);
    unsigned long bytes_read;
    long header_offset = -1;

    while ((bytes_read = fread(buffer, sizeof(char), 1024, input_file)) > 0)
    {
        for (size_t i = 0; i < bytes_read - 2; i++)
        {
            // printf("bytes read is %ld\n", bytes_read);
            if (memcmp(&buffer[i], HEADER, 2) == 0)
            {
                printf("Found JPEG header at offset: %lu\n", ftell(input_file) - bytes_read + i);
                header_offset = ftell(input_file) - bytes_read + i;
            }
            else if (memcmp(&buffer[i], FOOTER, 2) == 0)
            {
                printf("Found JPEG footer at offset: %lu\n", ftell(input_file) - bytes_read + i);
                if (header_offset != -1)
                {
                    printf("Carving JPEG from offset %ld to %lu\n", header_offset, ftell(input_file) - bytes_read + i + 2);
                    header_offset = -1;
                }
            }
        }
    }

    fclose(input_file);
    return 0;
}