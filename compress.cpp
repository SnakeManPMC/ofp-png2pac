#include "stdafx.h"

unsigned char *window_start, *window_end;
unsigned char *match_start;

int find_match(unsigned char *win_pos, unsigned char *in_pos, unsigned char **tmp_match_start) {
	int match_len = 0;

	*tmp_match_start = NULL;

	while (win_pos < window_end) {
		if (*win_pos == *in_pos) {
			*tmp_match_start = win_pos;
			break;
		}
		win_pos++;
	}
	if (*tmp_match_start == NULL) return 0;

	while (*win_pos++ == *in_pos++ && win_pos < window_end && match_len < 18)
		match_len++;

	return match_len;
}

int find_longest_match (unsigned char *in_pos) {
	unsigned char *win_pos;
	unsigned char *tmp_match_start;

	int match_len = 0;

	win_pos = window_start;

	while (win_pos < window_end) {
		int tmp_len = find_match(win_pos, in_pos, &tmp_match_start);

		if (tmp_len > match_len) {
			match_len = tmp_len;
			match_start = tmp_match_start;
			win_pos = match_start + match_len;
		} 
		if (tmp_len == 0) break;

		win_pos++;
	}

	return match_len;
}

int compress(unsigned char *output, unsigned char *input, int input_length) {
	unsigned char *input_pos = input;
	unsigned char *blocktype_pos;
	int block_count;
	int out_bytes = 0;
	int match_len;
	int checksum;

	window_start = window_end = input;

	blocktype_pos = output;
	*blocktype_pos = 0;
	block_count = 0;

	output++;
	out_bytes++;

	while (input_pos < input + input_length ) {
		char current = *input_pos;

		if (block_count > 7) {
			*output = 0x00;
			blocktype_pos = output;
			output++;
			out_bytes++;
			block_count = 0;
		}

		if ((match_len = find_longest_match(input_pos)) > 2) {
			int rel_pos = (int)(input_pos - match_start);

			*blocktype_pos |= (0x00 << block_count++);

			*output++ = rel_pos & 0xff;
			*output++ = ((rel_pos >> 4) & 0xf0) | ((match_len - 3) & 0x0f);
			out_bytes += 2;

			input_pos += match_len;
		} else {
			*blocktype_pos |= (0x01 << block_count++);
			*output = *input_pos;

			input_pos++;
			output++;
			out_bytes++;
		}
		window_end = input_pos - 1;

		if (window_end - window_start > 4095)
			window_start = window_end - 4095;

	}

	checksum = 0;

	char *checksum_pos = (char *)input;
	while (checksum_pos < (char *)input + input_length) {
		checksum += *checksum_pos++;
	}

	// printf("checksum = %08d\n", checksum);

	memcpy(output, &checksum, 4);
	out_bytes += 4;

	printf("compression: %d/%d, %2.2f%%\n", out_bytes, input_length, 100 * (float)out_bytes / input_length);

	return out_bytes;
}

