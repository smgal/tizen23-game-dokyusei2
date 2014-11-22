
#ifndef __FLAT_BOARD_H__
#define __FLAT_BOARD_H__

#include "pixel_format.h"

namespace flat_board
{
	template<typename BaseType>
	class Optional
	{
	public:
		Optional()
			: m_initialized(false)
		{
		}

		Optional(const BaseType& t)
			: m_initialized(false)
		{
			reset(t);
		}

		~Optional()
		{
			reset();
		}

		BaseType& operator*()
		{
			// assert(m_initialized);
			return *static_cast<BaseType*>(_address());
		}

		const BaseType& operator*() const
		{
			// assert(m_initialized);
			return *static_cast<const BaseType*>(_address());
		}

		BaseType* get(void)
		{
			// assert(m_initialized);
			return static_cast<BaseType*>(_address());
		}

		const BaseType* get(void) const
		{
			// assert(m_initialized);
			return static_cast<const BaseType*>(_address());
		}

		void reset(void)
		{
			if (m_initialized)
			{
				static_cast<BaseType*>(_address())->BaseType::~BaseType();
				m_initialized = false;
			}
		}

		void reset(const BaseType& t)
		{
			reset();
			new (_address()) BaseType(t);
			m_initialized = true;
		}

		bool valid(void) const
		{
			return m_initialized;
		}

		bool invalid(void) const
		{
			return !m_initialized;
		}

	private:
		bool     m_initialized;
		BaseType m_storage;

		Optional(const Optional&);
		Optional& operator=(const Optional&);

		void* _address(void)
		{
			return &m_storage;
		}
		const void* _address(void) const
		{
			return &m_storage;
		}
	};

	////////////////////////////////////////////////////////////////////////////

	template <PIXELFORMAT pixel_format>
	struct PixelFormatTraits
	{
	};

	template <>
	struct PixelFormatTraits<PIXELFORMAT_RGB565>
	{
		typedef unsigned short Pixel;

		struct Annex
		{
		};

		static Pixel convertColor(unsigned long color_32bits)
		{
			return Pixel((color_32bits & 0x00F80000) >> 8 |
			             (color_32bits & 0x0000FC00) >> 5 |
			             (color_32bits & 0x000000F8) >> 3);
		}
	};

	template <>
	struct PixelFormatTraits<PIXELFORMAT_ARGB8888>
	{
		typedef unsigned long Pixel;

		struct Annex
		{
		};

		static Pixel convertColor(unsigned long color_32bits)
		{
			return color_32bits;
		}
	};

	template <PIXELFORMAT pixel_format>
	struct PixelInfo
	{
		typedef typename PixelFormatTraits<pixel_format>::Pixel Pixel;
		typedef typename PixelFormatTraits<pixel_format>::Annex Annex;

		static Pixel convertColor(unsigned long color_32bits)
		{
			return PixelFormatTraits<pixel_format>::convertColor(color_32bits);
		}
	};

	struct BufferDesc
	{
		PIXELFORMAT   pixel_format;
		int           width;
		int           height;
		int           depth;
		bool          has_color_key;
		unsigned long color_key;
	};

	struct LockDesc
	{
		void*         p_buffer;
		long          bytes_per_line;
	};

	template <PIXELFORMAT pixel_format>
	class FlatBoard
	{
	public:
		typedef typename PixelInfo<pixel_format>::Pixel Pixel;

		typedef void (*FnFillRect)(Pixel* p_dest, int w, int h, int dest_pitch, Pixel color, unsigned long alpha);
		typedef void (*FnBitBlt)(Pixel* p_dest, int w, int h, int dest_pitch, Pixel* p_sour, int sour_pitch, unsigned long alpha, const Optional<Pixel>& chroma_key);

		FlatBoard()
			: m_fn_fill_rect(m_fillRect)
			, m_fn_bit_blt(m_bitBlt)
		{
			m_buffer_desc.pixel_format  = PIXELFORMAT_INVALID;
			m_buffer_desc.width         = 0;
			m_buffer_desc.height        = 0;
			m_buffer_desc.depth         = 0;
			m_buffer_desc.has_color_key = false;
			m_buffer_desc.color_key     = 0;

			m_lock_desc.p_buffer        = 0;
			m_lock_desc.bytes_per_line  = 0;
		}

		FlatBoard(Pixel* p_buffer, int width, int height, int pitch)
			: m_fn_fill_rect(m_fillRect)
			, m_fn_bit_blt(m_bitBlt)
		{
			m_buffer_desc.pixel_format  = pixel_format;
			m_buffer_desc.width         = width;
			m_buffer_desc.height        = height;
			m_buffer_desc.depth         = sizeof(Pixel) * 8;
			m_buffer_desc.has_color_key = false;
			m_buffer_desc.color_key     = 0;

			m_lock_desc.p_buffer        = (void*)p_buffer;
			m_lock_desc.bytes_per_line  = pitch * sizeof(Pixel);
		}

		FlatBoard(Pixel* p_buffer, int width, int height, int pitch, Pixel chroma_key)
			: m_chroma_key(chroma_key)
			, m_fn_fill_rect(m_fillRect)
			, m_fn_bit_blt(m_bitBlt)
		{
			m_buffer_desc.pixel_format  = pixel_format;
			m_buffer_desc.width         = width;
			m_buffer_desc.height        = height;
			m_buffer_desc.depth         = sizeof(Pixel) * 8;
			m_buffer_desc.has_color_key = true;
			m_buffer_desc.color_key     = chroma_key;

			m_lock_desc.p_buffer        = (void*)p_buffer;
			m_lock_desc.bytes_per_line  = pitch * sizeof(Pixel);
		}

		void fillRect(int x, int y, int w, int h, Pixel color, unsigned long alpha = 255)
		{
			if (m_lock_desc.p_buffer == 0)
				return;

			if (!m_cropInBoundary(x, y, w, h))
				return;

			if (m_chroma_key.valid())
			{
				if (*m_chroma_key == color)
					color = (color) ? (color-1) : (color+1);
			}

			m_fn_fill_rect(m_getBufferPointerAt(x, y), w, h, m_lock_desc.bytes_per_line / sizeof(Pixel), color, alpha);
		}

		void bitBlt(int x_dest, int y_dest, FlatBoard* p_src_flat_board, int x_sour, int y_sour, int w_sour, int h_sour, unsigned long alpha = 255)
		{
			if (m_lock_desc.p_buffer == 0)
				return;

			if (p_src_flat_board == 0 || p_src_flat_board->m_lock_desc.p_buffer == 0)
				return;

			{
				int x = x_sour;
				int y = y_sour;

				if (!p_src_flat_board->m_cropInBoundary(x_sour, y_sour, w_sour, h_sour))
					return;

				x_dest += (x_sour - x);
				y_dest += (y_sour - y);
			}

			{
				int x = x_dest;
				int y = y_dest;

				if (!m_cropInBoundary(x_dest, y_dest, w_sour, h_sour))
					return;

				x_sour += (x_dest - x);
				y_sour += (y_dest - y);
			}

			m_fn_bit_blt(m_getBufferPointerAt(x_dest, y_dest), w_sour, h_sour, m_lock_desc.bytes_per_line / sizeof(Pixel), p_src_flat_board->m_getBufferPointerAt(x_sour, y_sour), p_src_flat_board->m_lock_desc.bytes_per_line * 8 / p_src_flat_board->m_buffer_desc.depth, alpha, p_src_flat_board->m_chroma_key);
		}

		BufferDesc getBufferDesc(void) const
		{
			return m_buffer_desc;
		}

		FlatBoard<pixel_format>& operator<<(FnFillRect fn_fill_rect)
		{
			m_fn_fill_rect = (fn_fill_rect) ? fn_fill_rect : m_fillRect;
			return *this;
		}

		FlatBoard<pixel_format>& operator<<(FnBitBlt fn_bit_blt)
		{
			m_fn_bit_blt = (fn_bit_blt) ? fn_bit_blt : m_bitBlt;
			return *this;
		}

		inline FnFillRect getDefaultFillRect(void)
		{
			return m_fillRect;
		}

		inline FnBitBlt getDefaultBitBlt(void)
		{
			return m_bitBlt;
		}

		void setChromaKey(Pixel chroma_key)
		{
			m_buffer_desc.has_color_key = true;
			m_buffer_desc.color_key     = chroma_key;

			m_chroma_key.reset(chroma_key);
		}

		void resetChromaKey(void)
		{
			m_buffer_desc.has_color_key = false;
			m_buffer_desc.color_key     = 0;

			m_chroma_key.reset();
		}

	private:
		BufferDesc      m_buffer_desc;
		LockDesc        m_lock_desc;

		Optional<Pixel> m_chroma_key;
		FnFillRect      m_fn_fill_rect;
		FnBitBlt        m_fn_bit_blt;

		bool m_cropInBoundary(int& x1, int& y1, int& w, int& h)
		{
			int x2 = x1 + w;
			int y2 = y1 + h;

			if (x1 < 0)
				x1 = 0;
			if (y1 < 0)
				y1 = 0;

			if (x2 > m_buffer_desc.width)
				x2 = m_buffer_desc.width;
			if (y2 > m_buffer_desc.height)
				y2 = m_buffer_desc.height;

			w = x2 - x1;
			h = y2 - y1;

			return (w >= 0) && (h >= 0);
		}

		inline Pixel* m_getBufferPointerAt(int x, int y) const
		{
			return reinterpret_cast<Pixel*>((unsigned char*)m_lock_desc.p_buffer + y * m_lock_desc.bytes_per_line + x * sizeof(Pixel));
		}

		static void m_fillRect(Pixel* p_dest, int w, int h, int dest_pitch, Pixel color, unsigned long alpha);
		static void m_bitBlt(Pixel* p_dest, int w, int h, int dest_pitch, Pixel* p_sour, int sour_pitch, unsigned long alpha, const Optional<Pixel>& chroma_key);
	};

	template <>
	inline void FlatBoard<PIXELFORMAT_ARGB8888>::m_fillRect(unsigned long* p_dest_32, int w, int h, int dest_pitch, unsigned long color, unsigned long alpha)
	{
		alpha += (alpha >> 7);
		alpha  = ((alpha * (color >> 24)) << 16) & 0xFF000000;
		color &= 0x00FFFFFF;

		while (--h >= 0)
		{
			int copy = w;

			while (--copy >= 0)
				*p_dest_32++ = Pixel((color & 0x00FFFFFF) | alpha);

			p_dest_32 += (dest_pitch - w);
		}
	}

	template <>
	inline void FlatBoard<PIXELFORMAT_ARGB8888>::m_bitBlt(unsigned long* p_dest_32, int w, int h, int dest_pitch, unsigned long* p_sour_32, int sour_pitch, unsigned long alpha, const Optional<unsigned long>& chroma_key)
	{
		if (chroma_key.valid())
		{
			unsigned long color_key = *chroma_key;

			alpha  += (alpha >> 7);
			alpha <<= 16;

			Pixel gray;

			while (--h >= 0)
			{
				for (int copy = w; --copy >= 0; ++p_dest_32, ++p_sour_32)
				{
					if (*p_sour_32 != color_key)
					{
						gray = Pixel(alpha * (*p_sour_32 >> 24));
						*p_dest_32 = (*p_sour_32 & 0x00FFFFFF) | (gray & 0xFF000000);
					}
				}
				p_dest_32 += (dest_pitch - w);
				p_sour_32 += (sour_pitch - w);
			}
		}
		else
		{
			alpha  += (alpha >> 7);
			alpha <<= 16;

			Pixel gray;

			while (--h >= 0)
			{
				int copy = w;

				while (--copy >= 0)
				{
					gray = Pixel(alpha * (*p_sour_32 >> 24));
					*p_dest_32++ = (*p_sour_32++ & 0x00FFFFFF) | (gray & 0xFF000000);
				}
				p_dest_32 += (dest_pitch - w);
				p_sour_32 += (sour_pitch - w);
			}
		}
	}

	template <>
	inline void FlatBoard<PIXELFORMAT_RGB565>::m_fillRect(unsigned short* p_dest_16, int w, int h, int dest_pitch, unsigned short color, unsigned long alpha)
	{
		unsigned short color16bit = (unsigned short)color;

		while (--h >= 0)
		{
			int copy = w;

			while (--copy >= 0)
				*p_dest_16++ = color16bit;

			p_dest_16 += (dest_pitch - w);
		}
	}

	template <>
	inline void FlatBoard<PIXELFORMAT_RGB565>::m_bitBlt(unsigned short* p_dest_16, int w, int h, int dest_pitch, unsigned short* p_sour_16, int sour_pitch, unsigned long alpha, const Optional<unsigned short>& chroma_key)
	{
		if (chroma_key.valid())
		{
			unsigned short color_key = *chroma_key;

			while (--h >= 0)
			{
				for (int copy = w; --copy >= 0; ++p_dest_16, ++p_sour_16)
				{
					if (*p_sour_16 != color_key)
						*p_dest_16 = *p_sour_16;
				}
				p_dest_16 += (dest_pitch - w);
				p_sour_16 += (sour_pitch - w);
			}
		}
		else
		{
			while (--h >= 0)
			{
				int copy = w;
				while (--copy >= 0)
				{
					*p_dest_16++ = *p_sour_16++;
				}
				p_dest_16 += (dest_pitch - w);
				p_sour_16 += (sour_pitch - w);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////

	template <typename Pixel>
	bool loadPCX(const unsigned char* p_stream, unsigned long stream_size, Pixel*& p_out_buffer, int& out_width, int& out_height);

	template <>
	bool loadPCX<unsigned long>(const unsigned char* p_stream, unsigned long stream_size, unsigned long*& p_out_buffer, int& out_width, int& out_height);
}

#endif // #ifndef __FLAT_BOARD_H__
