#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#define CHUNK_SIZE 4096

const unsigned char HEADER[] = {0xFF, 0xD8, 0xFF};  // JPEG start
const unsigned char FOOTER[] = {0xFF, 0xD9};        // JPEG end
long files_carved = 0;

static off_t carve(int fd, off_t start, off_t end, const char *out_dir, long index)
{
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s/carved_%ld.jpg", out_dir, index);
    FILE* output_file = fopen(output_filename, "wb");
    if (!output_file)
    {
        perror(output_filename);
        return -1;
    }

    off_t bytes_to_read = end - start;
    unsigned char copy_buffer[4096];
    off_t bytes_copied = 0;

    while (bytes_copied < bytes_to_read) {
        size_t chunk = sizeof(copy_buffer);
        if (bytes_to_read - bytes_copied < (off_t)chunk) {
            chunk = (size_t)(bytes_to_read - bytes_copied);
        }
        ssize_t got = pread(fd, copy_buffer, chunk, start + bytes_copied);
        if (got <= 0)
        {
            if (got < 0) perror("pread");
            fprintf(stderr, "Read failed at offset %lld while carving %s\n",
                    (long long)(start + bytes_copied), output_filename);
            break;
        }
        if (fwrite(copy_buffer, 1, (size_t)got, output_file) != (size_t)got)
        {
            perror(output_filename);
            break;
        }
        bytes_copied += got;
    }
    if (fclose(output_file) != 0)
        perror(output_filename);
    if (bytes_copied < bytes_to_read)
        fprintf(stderr, "Warning: %s is incomplete (%lld of %lld bytes)\n",
                output_filename, (long long)bytes_copied, (long long)bytes_to_read);
    printf("Carved %lld bytes to %s\n", (long long)bytes_copied, output_filename);
    return bytes_copied;
}

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
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening input device");
        return 1;
    }

    unsigned char* buffer = malloc(CHUNK_SIZE);
    if (!buffer)
    {
        perror("Error allocating buffer");
        close(fd);
        return 1;
    }
    ssize_t bytes_read;
    off_t pos = 0;
    off_t header_offset = -1;

    while ((bytes_read = pread(fd, buffer, CHUNK_SIZE, pos)) > 0)
    {
        off_t chunk_start = pos;

        for (ssize_t i = 0; i + 3 < bytes_read; i++)
        {
            if (memcmp(&buffer[i], HEADER, 3) == 0 && has_jpeg_marker(buffer[i + 3]))
            {
                header_offset = chunk_start + i;
                printf("Found JPEG header at offset: %lld\n", (long long)header_offset);
            }
            else if (memcmp(&buffer[i], FOOTER, 2) == 0)
            {
                off_t footer_offset = chunk_start + i;

                if (header_offset != -1 && footer_offset > header_offset)
                {
                    printf("Carving JPEG from offset %lld to %lld\n",
                           (long long)header_offset, (long long)(footer_offset + 2));
                    if (carve(fd, header_offset, footer_offset + 2, argv[2], files_carved) < 0)
                    {
                        free(buffer);
                        close(fd);
                        return 1;
                    }
                    files_carved++;
                    header_offset = -1;
                }
            }
        }

        if (bytes_read <= 3)
            break;
        pos += bytes_read - 3;
    }

    int rc = 0;
    if (bytes_read < 0)
    {
        perror("Error reading input");
        rc = 1;
    }

    free(buffer);
    close(fd);
    return rc;
}