#include "stdafx.h"
#include "HW1Decomp.h"

int getc(HANDLE stream)
{
	DWORD dwRead;
	char c;
	ReadFile(stream, &c, 1, &dwRead, NULL);

	return c;
}

int bitioFileInputBit( BIT_FILE *bit_file )
{
	int value;

	if ( bit_file->mask == 0x80 ) {
		bit_file->rack = getc( bit_file->file );
		++bit_file->index;
	}
	value = bit_file->rack & bit_file->mask;
	bit_file->mask >>= 1;
	if ( bit_file->mask == 0 )
		bit_file->mask = 0x80;
	return( value ? 1 : 0 );
}

unsigned long bitioFileInputBits( BIT_FILE *bit_file, int bit_count )
{
	unsigned long mask;
	unsigned long return_value;

	mask = 1L << ( bit_count - 1 );
	return_value = 0;
	while ( mask != 0) {
		if ( bit_file->mask == 0x80 ) {
			bit_file->rack = getc( bit_file->file );
			++bit_file->index;
		}
		if ( bit_file->rack & bit_file->mask )
			return_value |= mask;
		mask >>= 1;
		bit_file->mask >>= 1;
		if ( bit_file->mask == 0 )
			bit_file->mask = 0x80;
	}
	return( return_value );
}

//
//  read from an existing, open file stream (in bit mode)
//
//	don't use regular char-mode input on this stream until after a
//	call to bitioFileInputStop()
//
BIT_FILE     *bitioFileInputStart(HANDLE fp)
{
	BIT_FILE *bit_file;

	bit_file = (BIT_FILE *) calloc( 1, sizeof( BIT_FILE ) );
	if ( bit_file == NULL )
		return( bit_file );
	bit_file->file = fp;
	bit_file->rack = 0;
	bit_file->mask = 0x80;
	bit_file->pacifier_counter = 0;
	bit_file->index = 0;
	return( bit_file );
}

//
//	switch back to regular (byte-oriented) input from the stream
//
//	returns bytes read in bit mode
//
int bitioFileInputStop(BIT_FILE *bit_file)
{
	int ret = bit_file->index;
	free( (char *) bit_file );
	return ret;
}

#define INDEX_BIT_COUNT      12
#define LENGTH_BIT_COUNT     4
#define WINDOW_SIZE          ( 1 << INDEX_BIT_COUNT )
#define BREAK_EVEN           ( ( 1 + INDEX_BIT_COUNT + LENGTH_BIT_COUNT ) / 9 )
#define END_OF_STREAM        0
#define MOD_WINDOW( a )      ( ( a ) & ( WINDOW_SIZE - 1 ) )

unsigned char window[ WINDOW_SIZE ];

//
//  Expands from a bit file to a buffer.
//
//  returns
//      -1 if there was an error
//      size of expanded data in output if successful
//
int lzssExpandFileToBuffer(BIT_FILE *input, char *output, int outputSize)
{
	int i;
	int current_position;
	int c;
	int match_length;
	int match_position;
	char *outBuffer = output;
	int size = 0;

	current_position = 1;
	for ( ; ; ) {
		if (bitioFileInputBit(input))
		{
			c = (int)bitioFileInputBits(input, 8);
			*(outBuffer++) = c;
			window[current_position] = (unsigned char)c;
			current_position = MOD_WINDOW(current_position + 1);
		} else {
			match_position = (int)bitioFileInputBits(input, INDEX_BIT_COUNT);
			if (match_position == END_OF_STREAM)
				break;
			match_length = (int)bitioFileInputBits(input, LENGTH_BIT_COUNT);
			match_length += BREAK_EVEN;
			for ( i = 0 ; i <= match_length ; i++ )
			{
				c = window[MOD_WINDOW(match_position + i)];
				*(outBuffer++) = c;
				window[current_position] = (unsigned char)c;
				current_position = MOD_WINDOW(current_position + 1);
			}
		}
	}

	return (outBuffer - output);
}
