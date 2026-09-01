#include <stdio.h>

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
    fclose(input_file);
    return 0;
}