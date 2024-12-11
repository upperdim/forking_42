#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>


#include <pthread.h>
#include <immintrin.h>
#include <sys/stat.h>
// #include <sys/mmap.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define FALSE 0

typedef char i8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef int i32;
typedef unsigned u32;
typedef unsigned long u64;

#define PRINT_ERROR(cstring) write(STDERR_FILENO, cstring, sizeof(cstring) - 1)

#pragma pack(1)
struct bmp_header {
	// Note: header
	i8  signature[2]; // should equal to "BM"
	u32 file_size;
	u32 unused_0;
	u32 data_offset;

	// Note: info header
	u32 info_header_size;
	u32 width; // in px
	u32 height; // in px
	u16 number_of_planes; // should be 1
	u16 bit_per_pixel; // 1, 4, 8, 16, 24 or 32
	u32 compression_type; // should be 0
	u32 compressed_image_size; // should be 0
	// Note: there are more stuff there but it is not important here
};

struct file_content {
	i8*   data;
	u32   size;
};

struct file_content   read_entire_file(char* filename) {
	char* file_data = 0;
	unsigned long	file_size = 0;

	int input_file_fd = open(filename, O_RDONLY);
	if (input_file_fd < 0) {
		return (struct file_content){file_data, file_size};
	}

	struct stat input_file_stat = {0};
	stat(filename, &input_file_stat);
	file_size = input_file_stat.st_size;
	file_data = mmap(0, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, input_file_fd, 0);
	
	close(input_file_fd);
	return (struct file_content){file_data, file_size};
}

u32 get_row(struct bmp_header *header, size_t i) {
	u32 bmp_row = ((i - header->data_offset) / 4) / header->width;
	return header->height - bmp_row - 1;
}

u32 get_col(struct bmp_header *header, size_t i) {
	u32 bmp_col = ((header->data_offset + i) % (header->width)) / 4;
	return bmp_col;
}

size_t get_px_idx(struct bmp_header *header, u32 row, u32 col) {
	row = header->height - 1 - row;
	size_t px_idx = header->data_offset + (col * 4) + (row * 4 * header->width);
	return px_idx;
}

int cmp_color_line_horiz(struct file_content *file_content, struct bmp_header *bmp_header, u32 start_r, u32 end_r, u32 col, u8 equals_red, u8 equals_green, u8 equals_blue) {
	for (size_t curr_r = start_r; curr_r <= end_r; ++curr_r) {
		u32 check_px_idx = get_px_idx(bmp_header, curr_r, col);
		if (file_content->data[check_px_idx] != equals_blue
			|| file_content->data[check_px_idx] + 1 != equals_green
			|| file_content->data[check_px_idx] + 2 != equals_red) {
				return FALSE;
		}
	}
	return TRUE;
}

int main(int argc, char** argv) {
	if (argc != 2) { PRINT_ERROR("Usage: decode <input_filename>\n"); return 1;}
	struct file_content file_content = read_entire_file(argv[1]);
	if (file_content.data == NULL) { PRINT_ERROR("Failed to read file\n"); return 1;}
	struct bmp_header* header = (struct bmp_header*) file_content.data;
	// printf("signature: %.2s\nfile_size: %u\ndata_offset: %u\ninfo_header_size: %u\nwidth: %u\nheight: %u\nplanes: %i\nbit_per_px: %i\ncompression_type: %u\ncompression_size: %u\n\n", header->signature, header->file_size, header->data_offset, header->info_header_size, header->width, header->height, header->number_of_planes, header->bit_per_pixel, header->compression_type, header->compressed_image_size);

	u32 pattern_row = 0;
	u32 pattern_col = 0;

	u32 msg_len = -1;

	for (u32 img_row = 0; img_row < header->height; ++img_row) {
		for (u32 img_col = 0; img_col < header->width; ++img_col) {
			u32 bmp_px_idx = get_px_idx(header, img_row, img_col);

			u8 blue  = file_content.data[bmp_px_idx + 0];
			u8 green = file_content.data[bmp_px_idx + 1];
			u8 red   = file_content.data[bmp_px_idx + 2];

			if (blue == 127 && green == 188 && red == 217) {
				// printf("found color at img_row=%u, img_col=%u\n", img_row, img_col);

				int did_find_pattern = TRUE;
				// Check vertical
				for (int r_offset = 0; r_offset <= 7; ++r_offset) {

					u32 search_px_idx = get_px_idx(header, img_row + r_offset, img_col);

					u8 check_blue = file_content.data[search_px_idx + 0];
					u8 check_green = file_content.data[search_px_idx + 1];
					u8 check_red = file_content.data[search_px_idx + 2];

					// printf("checking r offset %d r=%u g=%u b=%u\n", r_offset, check_red, check_green, check_blue);

					if (check_red != 217 || check_green != 188 || check_blue != 127) {
						// printf("vert check failed\n");
						did_find_pattern = FALSE;
						break;
					}
				}

				// Check horizontal
				for (int c_offset = 0; c_offset <= 6; ++c_offset) {

					u32 search_px_idx = get_px_idx(header, img_row, img_col + c_offset);

					u8 check_blue = file_content.data[search_px_idx + 0];
					u8 check_green = file_content.data[search_px_idx + 1];
					u8 check_red = file_content.data[search_px_idx + 2];

					// printf("checking c offset %d r=%u g=%u b=%u\n", c_offset, check_red, check_green, check_blue);
				
					if (check_red != 217 || check_green != 188 || check_blue != 127) {
						// printf("horiz check failed\n");
						did_find_pattern = FALSE;
						break;
					}
				}

				if (did_find_pattern) {
					// printf("Pattern found at (%u, %u)\n", img_row, img_col);
					pattern_row = img_row;
					pattern_col = img_col;
					
					u32 msg_len_row = pattern_row;
					u32 msg_len_col = pattern_col + 7;

					u32 msg_len_px_idx = get_px_idx(header, msg_len_row, msg_len_col);

					u8 msg_len_px_blue  = file_content.data[msg_len_px_idx + 0];
					// u8 msg_len_px_green = file_content.data[msg_len_px_idx + 1];
					u8 msg_len_px_red   = file_content.data[msg_len_px_idx + 2];

					msg_len = msg_len_px_blue + msg_len_px_red;

					// printf("msg len px = [%u,%u,%u]\n", msg_len_px_red, msg_len_px_green, msg_len_px_blue);
				}
			}
		}
	}

	// printf("msg len = %d\n", msg_len);
	u32 printed_count = 0;

	for (u32 r = pattern_row+2; r < header->height; ++r) {
		for (u32 c = pattern_col+2; c < header->width; ++c) {
			u32 curr_px_idx = get_px_idx(header, r, c);

			u8 blue  = file_content.data[curr_px_idx + 0];
			u8 green = file_content.data[curr_px_idx + 1];
			u8 red   = file_content.data[curr_px_idx + 2];

			if (printed_count >= msg_len) {
				exit(0);
			}

			

			printf("%c", blue);
			printed_count += 1;
			if (printed_count >= msg_len) {
				exit(0);
			}
			printf("%c", green);
			printed_count += 1;
			if (printed_count >= msg_len) {
				exit(0);
			}
			printf("%c", red);
			printed_count += 1;
			if (printed_count >= msg_len) {
				exit(0);
			}
		}
	}

	return 0;
}
