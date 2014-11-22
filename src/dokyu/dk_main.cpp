
#include "../flat_board/flat_board.h"
#include "../flat_board/target_dep.h"
#include <new>
#include <string.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////
// local

namespace dokyu
{
	struct BufferDesc
	{
		void* p_start_address;
		int   width;
		int   height;
		int   bytes_per_line;
		int   bits_per_pixel;
	};

	void init(void);
	void done(void);
	bool loop(const BufferDesc& buffer_desc);
}

////////////////////////////////////////////////////////////////////////////////
// type definition

namespace dokyu
{
	typedef flat_board::FlatBoard<flat_board::PIXELFORMAT_ARGB8888> FlatBoard32;
	typedef unsigned long TargetPixel;
}

////////////////////////////////////////////////////////////////////////////////
// resource

namespace dokyu
{
	template <class T>
	class auto_deletor
	{
	public:
		auto_deletor(T* p_object = 0)
			: m_p_object(p_object)
		{
		}

		~auto_deletor(void)
		{
			delete[] m_p_object;
		}

		void operator()(T* p_object)
		{
			m_p_object = p_object;
		}

	private:
		T* m_p_object;

	};

	struct GameResource
	{
		FlatBoard32 res_waku;
		FlatBoard32 res_tile;
		FlatBoard32 res_tile_fg;
		FlatBoard32 res_yui;

		auto_deletor<FlatBoard32::Pixel> auto_release_waku;
		auto_deletor<FlatBoard32::Pixel> auto_release_tile;
		auto_deletor<FlatBoard32::Pixel> auto_release_tile_fg;
		auto_deletor<FlatBoard32::Pixel> auto_release_yui;
		auto_deletor<unsigned short>     auto_release_map;

		struct Map
		{
			Map()
				: x_size(0)
				, y_size(0)
				, p_map_data(0)
			{
			}

			short           x_size;
			short           y_size;
			unsigned short* p_map_data;

			int operator()(int x, int y)
			{
				x = (x < 0) ? x_size + x - 1 : x;
				y = (y < 0) ? y_size + y - 1 : y;
				x = (x > x_size - 1) ? x - x_size : x;
				y = (y > y_size - 1) ? y - y_size : y;

				return (p_map_data) ? p_map_data[y * x_size + x] & 0x7FFF : 0;
			}

			bool IsWalkable(int x, int y)
			{
				x = (x < 0) ? x_size + x - 1 : x;
				y = (y < 0) ? y_size + y - 1 : y;
				x = (x > x_size - 1) ? x - x_size : x;
				y = (y > y_size - 1) ? y - y_size : y;

				return (p_map_data) ? (p_map_data[y * x_size + x] & 0x8000) == 0 : false;
			}
		} res_map;

		GameResource()
		{
			typedef unsigned char  byte;
			typedef unsigned short word;

			unsigned long palette_data[16];

			// load waku
			{
				unsigned char* p_buffer = 0;
				int buffer_size = 0;

				{
					target::file_io::StreamReadFile file("WAKU.PCX");

					buffer_size = file.getSize();

					if (buffer_size > 0)
					{
						p_buffer = new unsigned char[buffer_size];
						file.read(p_buffer, buffer_size);
					}
				}

				if (p_buffer)
				{
					TargetPixel* p_buffer_target = 0;
					int          buffer_width;
					int          buffer_height;

					flat_board::loadPCX<TargetPixel>(p_buffer, buffer_size, p_buffer_target, buffer_width, buffer_height);

					delete[] p_buffer;

					new (&res_waku) FlatBoard32(static_cast<FlatBoard32::Pixel*>(p_buffer_target), buffer_width, buffer_height, buffer_width);
					auto_release_waku(p_buffer_target);

					res_waku.fillRect(120, 330, 280, 50, 0xFF000000);
				}
			}

			{
				unsigned char temp_palette_data[16][3];

				{
					target::file_io::StreamReadFile file("MATI1.PAL");
					file.read(temp_palette_data, sizeof(temp_palette_data));
				}

				for (int i = 0; i < 16; i++)
				{
				#if !defined(PIXELFORMAT_ABGR)
					unsigned long r = temp_palette_data[i][0] * 4;
					unsigned long g = temp_palette_data[i][1] * 4;
					unsigned long b = temp_palette_data[i][2] * 4;
				#else
					unsigned long b = temp_palette_data[i][0] * 4;
					unsigned long g = temp_palette_data[i][1] * 4;
					unsigned long r = temp_palette_data[i][2] * 4;
				#endif

					palette_data[i] = 0xFF000000 | r << 16 | g << 8 | b;
				}
			}

			{
				const int MAX_TILE   = 1000;
				const int IMAGE_SIZE = 134;

				unsigned char tile_data[MAX_TILE][IMAGE_SIZE];

				{
					target::file_io::StreamReadFile file("MATI1.IMG");
					file.read(tile_data, sizeof(tile_data));
				}

				word image_width  = word(tile_data[0][0]) + (word(tile_data[0][1]) << 8) + 1;
				word image_height = word(tile_data[0][2]) + (word(tile_data[0][3]) << 8) + 1;
				word image_pitch  = image_width / 8;

				{
					int buffer_width  = image_width * MAX_TILE;
					int buffer_height = image_height;
					int buffer_size   = buffer_width * buffer_height * sizeof(FlatBoard32::Pixel);

					{
						FlatBoard32::Pixel* p_buffer = new FlatBoard32::Pixel[buffer_width * buffer_height];
						new (&res_tile) FlatBoard32(p_buffer, buffer_width, buffer_height, buffer_width);
						auto_release_tile(p_buffer);
					}

					{
						FlatBoard32::Pixel* p_buffer = new FlatBoard32::Pixel[buffer_width * buffer_height];
						memset(p_buffer, 0, buffer_size);
						new (&res_tile_fg) FlatBoard32(p_buffer, buffer_width, buffer_height, buffer_width, 0x00000000);
						auto_release_tile_fg(p_buffer);
					}
				}

				for (int i = 0; i < MAX_TILE; i++)
				{
					byte* p_tile = (byte*)&tile_data[i][4];

					byte* p_tile_i = p_tile + image_pitch * 0;
					byte* p_tile_r = p_tile + image_pitch * 1;
					byte* p_tile_g = p_tile + image_pitch * 2;
					byte* p_tile_b = p_tile + image_pitch * 3;

					for (int y = 0; y < image_height; y++)
					{
						int base_index = image_pitch * 4 * y;

						for (int x = 0; x < image_width; x++)
						{
							int index = base_index + x / 8;
							int shift = x % 8;

							unsigned long palette = 0;

							palette |= (p_tile_r[index] & (0x80 >> shift)) ? 0x04 : 0;
							palette |= (p_tile_g[index] & (0x80 >> shift)) ? 0x02 : 0;
							palette |= (p_tile_b[index] & (0x80 >> shift)) ? 0x01 : 0;

							res_tile.fillRect(i * image_width + x, y, 1, 1, palette_data[palette]);

							if ((p_tile_i[index] & (0x80 >> shift)))
							{
								res_tile_fg.fillRect(i * image_width + x, y, 1, 1, palette_data[palette]);
							}
						}
					}
				}
			}

			{
				const int MAX_CHARACTER  = 8;
				const int CHARACTER_SIZE = 518;

				unsigned char tile_data[MAX_CHARACTER][CHARACTER_SIZE];

				{
					target::file_io::StreamReadFile file("YUI.IMG");
					file.read(tile_data, sizeof(tile_data));
				}

				word image_width  = word(tile_data[0][0]) + (word(tile_data[0][1]) << 8) + 1;
				word image_height = word(tile_data[0][2]) + (word(tile_data[0][3]) << 8) + 1;
				word image_pitch  = image_width / 8;

				{
					int buffer_width  = image_width * MAX_CHARACTER;
					int buffer_height = image_height;
					int buffer_size   = buffer_width * buffer_height * sizeof(FlatBoard32::Pixel);

					FlatBoard32::Pixel* p_buffer = new FlatBoard32::Pixel[buffer_width * buffer_height];

					memset(p_buffer, 0, buffer_size);

					new (&res_yui) FlatBoard32(p_buffer, buffer_width, buffer_height, buffer_width, 0x00000000);
					auto_release_yui(p_buffer);
				}

				for (int i = 0; i < MAX_CHARACTER; i++)
				{
					byte* p_tile = (byte*)&tile_data[i][4];

					byte* p_tile_i = p_tile + image_pitch * 0;
					byte* p_tile_r = p_tile + image_pitch * 1;
					byte* p_tile_g = p_tile + image_pitch * 2;
					byte* p_tile_b = p_tile + image_pitch * 3;

					for (int y = 0; y < image_height; y++)
					{
						int base_index = image_pitch * 4 * y;

						for (int x = 0; x < image_width; x++)
						{
							int index = base_index + x / 8;
							int shift = x % 8;

							if (!(p_tile_i[index] & (0x80 >> shift)))
							{
								unsigned long palette = 0;

								palette |= (p_tile_r[index] & (0x80 >> shift)) ? 0x04 : 0;
								palette |= (p_tile_g[index] & (0x80 >> shift)) ? 0x02 : 0;
								palette |= (p_tile_b[index] & (0x80 >> shift)) ? 0x01 : 0;

								res_yui.fillRect(i * image_width + x, y, 1, 1, palette_data[palette]);
							}
						}
					}
				}
			}

			{
				target::file_io::StreamReadFile file("MATI1.MAP");

				// little endian
				file.read(&res_map.x_size, 2);
				file.read(&res_map.y_size, 2);

				res_map.p_map_data = new unsigned short[res_map.x_size * res_map.y_size];

				// little endian
				file.read(res_map.p_map_data, res_map.x_size * res_map.y_size * sizeof(unsigned short));
				auto_release_map(res_map.p_map_data);
			}
		}
	};

	static GameResource* s_p_game_resource = 0;
}

////////////////////////////////////////////////////////////////////////////////
// object

namespace dokyu
{
	class Yui
	{
	public:
		Yui()
			: x(99)
			, y(25)
			, face(3)
		{
		}

		~Yui()
		{
		}

		void move(void)
		{
			face ^= 0x01;
		}

		void move(int x1, int y1)
		{
			if (this->isMoveable(x1, y1))
			{
				this->setFace(x1, y1);

				x += x1;
				y += y1;
			}
			else
			{
				if (x1 == -1)
				{
					if (this->isMoveable(x1, y1-1))
					{
						x1 = 0; y1 = -1;
					}
					else if (this->isMoveable(x1, y1+1))
					{
						x1 = 0; y1 = +1;
					}
				}
				else if (x1 == 1)
				{
					if (this->isMoveable(x1, y1+1))
					{
						x1 = 0; y1 = +1;
					}
					else if (this->isMoveable(x1, y1-1))
					{
						x1 = 0; y1 = -1;
					}
				}
				else if (y1 == -1)
				{
					if (this->isMoveable(x1+1, y1))
					{
						x1 = 1; y1 = 0;
					}
					else if (this->isMoveable(x1-1, y1))
					{
						x1 = -1; y1 = 0;
					}
				}
				else if (y1 == 1)
				{
					if (this->isMoveable(x1-1, y1))
					{
						x1 = -1; y1 = 0;
					}
					else if (this->isMoveable(x1+1, y1))
					{
						x1 = 1; y1 = 0;
					}
				}

				this->setFace(x1, y1);

				if (this->isMoveable(x1, y1))
				{
					x += x1;
					y += y1;
				}
			}
		}

		bool isMoveable(int x1, int y1)
		{
			if (s_p_game_resource)
				return (s_p_game_resource->res_map.IsWalkable(x+x1,y+y1+1) && s_p_game_resource->res_map.IsWalkable(x+x1+1,y+y1+1));
			else
				return false;
		}

		void setFace(int x1, int y1)
		{
			if (x1 < 0)
				face = 4 | (face & 0x01);
			if (x1 > 0)
				face = 6 | (face & 0x01);
			if (y1 < 0)
				face = 0 | (face & 0x01);
			if (y1 > 0)
				face = 2 | (face & 0x01);
		}

		int x;
		int y;
		int face;
	};

	static Yui s_yui;
}

////////////////////////////////////////////////////////////////////////////////
// local

namespace dokyu
{
	template <typename T>
	inline T REVISED_X(T x)
	{
		return x * 16 + 64;
	}

	template <typename T>
	inline T REVISED_Y(T y)
	{
		return y * 16;
	}
}

////////////////////////////////////////////////////////////////////////////////
// main

void dokyu::init()
{
	s_p_game_resource = new GameResource;
}

void dokyu::done()
{
	delete s_p_game_resource;
	s_p_game_resource = 0;
}

bool dokyu::loop(const BufferDesc& buffer_desc)
{
	if (buffer_desc.bits_per_pixel != 32)
		return false;

	GameResource::Map& map = s_p_game_resource->res_map;

	// object control
	{
		s_yui.move();

		target::InputDevice* p_input_device = target::getInputDevice();

		if (p_input_device)
		{
			p_input_device->update();

			if (p_input_device->checkKeyPressed(target::KEY_LEFT))
				s_yui.move(-1, 0);
			if (p_input_device->checkKeyPressed(target::KEY_RIGHT))
				s_yui.move(+1, 0);
			if (p_input_device->checkKeyPressed(target::KEY_UP))
				s_yui.move(0, -1);
			if (p_input_device->checkKeyPressed(target::KEY_DOWN))
				s_yui.move(0, +1);
		}
	}

	// display
	{
		const int GUIDE_WIDTH  = 640;
		const int GUIDE_HEIGHT = 400;

		int game_width  = (buffer_desc.width  > GUIDE_WIDTH ) ? GUIDE_WIDTH  : buffer_desc.width;
		int game_height = (buffer_desc.height > GUIDE_HEIGHT) ? GUIDE_HEIGHT : buffer_desc.height;
		int game_x      = (buffer_desc.width  - game_width ) >> 1;
		int game_y      = (buffer_desc.height - game_height) >> 1;
		int game_pitch  = (buffer_desc.bytes_per_line << 3) / buffer_desc.bits_per_pixel;

		FlatBoard32::Pixel* p_game_buffer = static_cast<FlatBoard32::Pixel*>(buffer_desc.p_start_address);
		p_game_buffer += ((game_pitch * game_y) + game_x);

		FlatBoard32 dest_board(p_game_buffer, game_width, game_height, game_pitch);

		{
			dest_board.bitBlt(0, 0, &s_p_game_resource->res_waku, 0, 0, GUIDE_WIDTH, GUIDE_HEIGHT);
		}

		{
			const int SCROLL_X_WIDE = 15;
			const int SCROLL_Y_WIDE = 8;

			for (int j = -SCROLL_Y_WIDE; j <= SCROLL_Y_WIDE+1; j++)
			for (int i = -SCROLL_X_WIDE; i <= SCROLL_X_WIDE+1; i++)
			{
				int map_index = map(i + s_yui.x, j + s_yui.y);
				dest_board.bitBlt(REVISED_X(i+SCROLL_X_WIDE), REVISED_Y(j+SCROLL_Y_WIDE), &s_p_game_resource->res_tile, 16*map_index, 0, 16, 16);
			}

			dest_board.bitBlt(REVISED_X(SCROLL_X_WIDE), REVISED_Y(SCROLL_Y_WIDE), &s_p_game_resource->res_yui, 32*s_yui.face, 0, 32, 32);

			dest_board.bitBlt(REVISED_X(SCROLL_X_WIDE  ), REVISED_Y(SCROLL_Y_WIDE  ), &s_p_game_resource->res_tile_fg, 16*map(s_yui.x  , s_yui.y  ), 0, 16, 16);
			dest_board.bitBlt(REVISED_X(SCROLL_X_WIDE+1), REVISED_Y(SCROLL_Y_WIDE  ), &s_p_game_resource->res_tile_fg, 16*map(s_yui.x+1, s_yui.y  ), 0, 16, 16);
			dest_board.bitBlt(REVISED_X(SCROLL_X_WIDE  ), REVISED_Y(SCROLL_Y_WIDE+1), &s_p_game_resource->res_tile_fg, 16*map(s_yui.x  , s_yui.y+1), 0, 16, 16);
			dest_board.bitBlt(REVISED_X(SCROLL_X_WIDE+1), REVISED_Y(SCROLL_Y_WIDE+1), &s_p_game_resource->res_tile_fg, 16*map(s_yui.x+1, s_yui.y+1), 0, 16, 16);
		}
	}

	return true;
}
