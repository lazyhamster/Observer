#include "stdafx.h"
#include "rtfcomp.h"

const char *lzfu_rtf_dictionary = \
	"{\\rtf1\\ansi\\mac\\deff0\\deftab720"
	"{\\fonttbl;}"
	"{\\f0\\fnil \\froman \\fswiss \\fmodern \\fscript \\fdecor MS Sans SerifSymbolArialTimes New RomanCourier"
	"{\\colortbl\\red0\\green0\\blue0\r\n\\par \\pard\\plain\\f0\\fs20\\b\\i\\u\\tab\\tx";

static uint16_t swap_bytes(uint16_t num)
{
	return (num >> 8) | (num << 8);
}

bool lzfu_decompress(uint8_t* compressedData, size_t compressedDataSize, uint8_t* uncompressedData, size_t uncompressedDataSize)
{
	uint8_t lz_buffer[4096] = {0};

	size_t compressed_data_iterator   = 0;
	size_t uncompressed_data_iterator = 0;
	uint16_t lz_buffer_iterator       = 0;
	uint16_t reference_offset         = 0;
	uint16_t reference_size           = 0;
	uint8_t flag_byte_bit_mask        = 0;
	uint8_t flag_byte                 = 0;

	memcpy(lz_buffer, lzfu_rtf_dictionary, 207);
	lz_buffer_iterator = 207;

	while( compressed_data_iterator < compressedDataSize )
	{
		flag_byte = compressedData[ compressed_data_iterator++ ];

		/* Check every bit in the chunk flag byte from LSB to MSB
		 */
		for( flag_byte_bit_mask = 0x01; flag_byte_bit_mask != 0x00; flag_byte_bit_mask <<= 1 )
		{
			if( compressed_data_iterator == compressedDataSize )
			{
				break;
			}
			// Each flag byte controls 8 literals/references, 1 per bit
			// Each bit is 1 for reference, 0 for literal
			if( ( flag_byte & flag_byte_bit_mask ) == 0 )
			{
				if ((compressed_data_iterator >= compressedDataSize) || (uncompressed_data_iterator >= uncompressedDataSize))
				{
					return false;
				}
				
				lz_buffer[ lz_buffer_iterator++ ] = compressedData[ compressed_data_iterator ];
				uncompressedData[ uncompressed_data_iterator++ ] = compressedData[ compressed_data_iterator ];

				compressed_data_iterator++;

				// Make sure the lz buffer iterator wraps around
				lz_buffer_iterator %= 4096;

				lz_buffer[lz_buffer_iterator] = 0;
			}
			else
			{
				if( ( compressed_data_iterator + 1 ) >= compressedDataSize )
				{
					return false;
				}

				reference_offset = *((uint16_t*) &(compressedData[compressed_data_iterator]));
				reference_offset = swap_bytes(reference_offset);

				compressed_data_iterator += 2;
				
				reference_size     = ( reference_offset & 0x000f ) + 2;
				reference_offset >>= 4;

				// A reference pointing to itself (the current position) marks the end of the data.
				if (reference_offset == lz_buffer_iterator)
				{
					break;
				}

				if (( uncompressed_data_iterator + reference_size - 1 ) >= uncompressedDataSize)
				{
					return false;
				}
				for (uint16_t reference_iterator = 0; reference_iterator < reference_size; reference_iterator++)
				{
					lz_buffer[ lz_buffer_iterator++ ] = lz_buffer[reference_offset];
					uncompressedData[ uncompressed_data_iterator++ ] = lz_buffer[reference_offset];

					reference_offset++;

					// Make sure the lz buffer iterator and reference offset wrap around
					lz_buffer_iterator %= 4096;
					reference_offset   %= 4096;

					lz_buffer[lz_buffer_iterator] = 0;
				}
			}
		}
	}  //while

	return true;
}

static uint32_t crc32_table[256];
static bool crc32_table_created = false;

static void init_crc32_table()
{
	uint32_t checksum    = 0;
	uint32_t table_index = 0;
	uint8_t bit_iterator = 0;

	for( table_index = 0; table_index < 256; table_index++ )
	{
		checksum = (uint32_t) table_index;

		for( bit_iterator = 0;
			bit_iterator < 8;
			bit_iterator++ )
		{
			if( checksum & 1 )
			{
				checksum = (uint32_t) 0xedb88320UL ^ ( checksum >> 1 );
			}
			else
			{
				checksum = checksum >> 1;
			}
		}
		crc32_table[table_index] = checksum;
	}
	crc32_table_created = true;
}

DWORD calculateWeakCrc32(uint8_t* buf, DWORD bufSize)
{
	if (!crc32_table_created)
		init_crc32_table();

	uint32_t table_index  = 0;
	DWORD crcVal = 0;

	for(size_t buffer_offset = 0; buffer_offset < bufSize; buffer_offset++)
	{
		table_index = ( crcVal ^ buf[buffer_offset] ) & 0x000000ffUL;

		crcVal = crc32_table[table_index] ^ (crcVal >> 8);
	}
	
	return crcVal;
}
