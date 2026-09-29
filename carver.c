#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

const unsigned char HEADER[] = {0xFF, 0xD8, 0xFF};  // JPEG start
const unsigned char FOOTER[] = {0xFF, 0xD9};        // JPEG end
long files_carved = 0;

bool has_jpeg_marker(unsigned char next_byte)
{
    return next_byte >= 0xE0 && next_byte <= 0xEF;
}

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

    unsigned char* buffer = malloc(4096);
    if (!buffer)
    {
        perror("Error allocating buffer");
        fclose(input_file);
        return 1;
    }
    unsigned long bytes_read;
    long header_offset = -1;

    while ((bytes_read = fread(buffer, sizeof(char), 4096, input_file)) > 0)
    {
        long chunk_start = ftell(input_file) - (long)bytes_read;

        for (size_t i = 0; i + 3 < bytes_read; i++)
        {
            if (memcmp(&buffer[i], HEADER, 3) == 0 && has_jpeg_marker(buffer[i + 3]))
            {
                header_offset = chunk_start + (long)i;
                printf("Found JPEG header at offset: %ld\n", header_offset);
            }
            else if (memcmp(&buffer[i], FOOTER, 2) == 0)
            {
                long footer_offset = chunk_start + (long)i;

                if (header_offset != -1 && footer_offset > header_offset)
                {
                    printf("Carving JPEG from offset %ld to %ld\n", header_offset, footer_offset + 2);
                    char output_filename[256];
                    snprintf(output_filename, sizeof(output_filename), "%s/carved_%ld.jpg", argv[2], files_carved++);
                    FILE* output_file = fopen(output_filename, "wb");

                    if (!output_file)
                    {
                        perror(output_filename);
                        free(buffer);
                        fclose(input_file);
                        return 1;
                    }

                    long bytes_to_read = (footer_offset + 2) - header_offset;
                    unsigned char copy_buffer[4096];
                    long bytes_copied = 0;

                    fseek(input_file, header_offset, SEEK_SET);

                    while (bytes_copied < bytes_to_read) {
                        size_t chunk = sizeof(copy_buffer);
                        if (bytes_to_read - bytes_copied < (long)chunk) {
                            chunk = bytes_to_read - bytes_copied;
                        }
                        size_t got = fread(copy_buffer, 1, chunk, input_file);
                        if (got == 0)
                        {
                            fprintf(stderr, "Read failed at offset %ld while carving %s\n",
                                    header_offset + bytes_copied, output_filename);
                            break;
                        }
                        if (fwrite(copy_buffer, 1, got, output_file) != got)
                        {
                            perror(output_filename);
                            break;
                        }
                        bytes_copied += got;
                    }
                    if (fclose(output_file) != 0)
                        perror(output_filename);
                    if (bytes_copied < bytes_to_read)
                        fprintf(stderr, "Warning: %s is incomplete (%ld of %ld bytes)\n",
                                output_filename, bytes_copied, bytes_to_read);
                    printf("Carved %ld bytes to %s\n", bytes_copied, output_filename);

                    header_offset = -1;
                    fseek(input_file, chunk_start + (long)bytes_read, SEEK_SET);
                }
            }
        }

        if (bytes_read <= 3)
            break;
        fseek(input_file, chunk_start + (long)bytes_read - 3, SEEK_SET);
    }

    free(buffer);
    fclose(input_file);
    return 0;
}