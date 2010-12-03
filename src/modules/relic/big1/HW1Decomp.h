#ifndef HW1Decomp_h__
#define HW1Decomp_h__

typedef struct bit_file {
	HANDLE file;
	unsigned char mask;
	int rack;
	int pacifier_counter;
	int index;  // like a file position, in bytes
} BIT_FILE;

typedef struct bit_buffer {
	char *buffer;
	int index;
	unsigned char mask;
	int rack;
	int pacifier_counter;
} BIT_BUFFER;

BIT_FILE     *bitioFileInputStart(HANDLE fp);
int			  bitioFileInputStop(BIT_FILE *bit_file);

int lzssExpandFileToBuffer(BIT_FILE *input, char *output, int outputSize);

#endif // HW1Decomp_h__