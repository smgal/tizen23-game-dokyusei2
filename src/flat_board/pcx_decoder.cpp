#include <string.h>
//??
#include <stdio.h>

#define g_printLog(s)

namespace flat_board
{
	namespace
	{
		typedef unsigned char  byte;
		typedef unsigned short word;

		struct Header
		{
			byte  manufacture;
			byte  version;
			byte  encording;
			byte  bit_per_pixel;
			short x1, y1, x2, y2;
			short h_dpi, v_dpi;
			byte  palette[16][3];
			byte  reserved;
			byte  planes;
			short bytes_per_line;
			short palette_info;
			short h_screensize;
			short v_screensize;
			byte  filler[54];
		};
	}

	template <typename Pixel>
	bool loadPCX(const unsigned char* p_stream, unsigned long stream_size, Pixel*& p_out_buffer, int& out_width, int& out_height)
	{
		return false;
	}

	template <>
	bool loadPCX<unsigned long>(const unsigned char* p_stream, unsigned long stream_size, unsigned long*& p_out_buffer, int& out_width, int& out_height)
	{
		typedef int _COMPILE_TIME_ASSERT[(sizeof(Header) == 128) ? 1 : -1];

		typedef unsigned long Pixel;
		typedef byte Palette256[256][3];

		const unsigned char* p_stream_end = p_stream + stream_size;

		Header header;

		memcpy(&header, p_stream, sizeof(header));

char sz_log[256];
sprintf(sz_log, "[SMGAL] %d, %d, %d, %d", sizeof(header), header.bit_per_pixel, header.x1, header.x2);
g_printLog(sz_log);

		p_stream += sizeof(header);
		p_stream_end += (header.bit_per_pixel == 8) ? -sizeof(Palette256) : 0;

		////////

		out_width    = (header.x2 + 1);
		out_height   = (header.y2 + 1);
		p_out_buffer = new Pixel[out_width * out_height];

		memset(p_out_buffer, 0, sizeof(Pixel) * out_width * out_height);

		int  x = 0;
		int  y = 0;
		byte plane = 1;

		while (p_stream < p_stream_end)
		{
			byte len;
			byte data;

			byte ch = *p_stream++;

			if ((ch & 0xC0) == 0xC0)
			{
				len = ch & 0x3F;
				// worry for out-of-range in the mal-coded PCX file
				data = *p_stream++;
			}
			else
			{
				len  = 1;
				data = ch;
			}

			while (len-- > 0)
			{
				if (header.planes == 1)
				{
					p_out_buffer[y * out_width + x] = data;

					if (++x >= int(header.bytes_per_line))
					{
						++y;
						x = 0;
					}

					if (y > header.y2)
					{
						len = 0;
						p_stream = p_stream_end;
						break;
					}
				}
				else
				{
					for (int shift = 0; shift < 8; shift++)
					{
						if (data & (0x80 >> shift))
						{
							p_out_buffer[y * out_width + 8*x + shift] |= plane;
						}
					}

					if (++x >= int(header.bytes_per_line))
					{
						plane <<= 1;
						if (plane == 0x10)
						{
							++y;
							plane = 1;
						}
						x = 0;
					}
				}
			}
		}

g_printLog("[SMGAL] #6");
		{
			Pixel palette[256];

			Palette256* pPalette = (header.bit_per_pixel == 8) ? (Palette256*)p_stream_end : (Palette256*)header.palette;

			for (int i = 0; i < (1 << (header.bit_per_pixel * header.planes)); i++)
			{
		#if 1
				// ARGB8888
			#if !defined(PIXELFORMAT_ABGR)
				Pixel r = (*pPalette)[i][0];
				Pixel g = (*pPalette)[i][1];
				Pixel b = (*pPalette)[i][2];
			#else
				Pixel b = (*pPalette)[i][0];
				Pixel g = (*pPalette)[i][1];
				Pixel r = (*pPalette)[i][2];
			#endif

				palette[i] = 0xFF000000 | (r << 16) | (g << 8) | (b);
		#else
				// RGB565
				Pixel r = (header.palette[i][0] >> 3) & 0x1F;
				Pixel g = (header.palette[i][1] >> 2) & 0x3F;
				Pixel b = (header.palette[i][2] >> 3) & 0x1F;

				palette[i] = (r << 11) | (g << 5) | (b);
		#endif
			}

			Pixel* p_buffer     = p_out_buffer;
			Pixel* p_buffer_end = p_out_buffer + out_width * out_height;

			while (p_buffer < p_buffer_end)
			{
				*p_buffer = palette[*p_buffer];
				++p_buffer;
			}
		}
g_printLog("[SMGAL] #7");

		return true;
	}
}
