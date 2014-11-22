
#include "../target_dep.h"

#include <cstring>

namespace
{
	int s_native_pressed_x = -1;
	int s_native_pressed_y = -1;

	bool s_is_pressed = false;
	int  s_pressed_x  = 0;
	int  s_pressed_y  = 0;

	struct TRect
	{
		int x1, y1, x2, y2;
	};

	bool s_InRect(int x, int y, const TRect& rect)
	{
		return (x >= rect.x1 && x < rect.x2 && y >= rect.y1 && y < rect.y2);
	}

	struct TRectInfo
	{
		TRect        rect;
		target::KEY key;
	} HIT_RECT[4];

	unsigned long s_GetButtons(void)
	{
		static bool s_is_first = true;

		if (s_is_first)
		{
			const int W = target::getScreenWidth();
			const int H = target::getScreenHeight();

			const int X1 = W * 0 / 3;
			const int X2 = W * 1 / 3;
			const int X3 = W * 2 / 3;
			const int X4 = W * 3 / 3;

			const int Y1 = H * 0 / 3;
			const int Y2 = H * 1 / 3;
			const int Y3 = H * 2 / 3;
			const int Y4 = H * 3 / 3;

			const TRectInfo TEMP_HIT_RECT[] =
			{
				// up
				{ { X2, Y1, X3, Y2 }, target::KEY_UP },
				// down
				{ { X2, Y3, X3, Y4 }, target::KEY_DOWN },
				// left
				{ { X1, Y2, X2, Y3 }, target::KEY_LEFT },
				// right
				{ { X3, Y2, X4, Y3 }, target::KEY_RIGHT },
			};

			memcpy(HIT_RECT, TEMP_HIT_RECT, sizeof(HIT_RECT));

			s_is_first = false;
		}

		s_is_pressed = false;

		unsigned long flags = 0;

		{
			if (s_native_pressed_x >= 0 && s_native_pressed_y >= 0)
			{
				int x = s_native_pressed_x;
				int y = s_native_pressed_y;

				if (!s_is_pressed)
				{
					s_is_pressed = true;
					s_pressed_x  = x;
					s_pressed_y  = y;
				}

				for (int key = 0; key < int(sizeof(HIT_RECT)/sizeof(HIT_RECT[0])); key++)
				{
					if (s_InRect(x, y, HIT_RECT[key].rect))
					{
						flags |= HIT_RECT[key].key;
					}
				}
			}
		}

		return flags;
	}
}

namespace target
{
	void setTouchPos(int x, int y)
	{
		s_native_pressed_x = x;
		s_native_pressed_y = y;
	}

	class CInputDevice: public InputDevice
	{
	public:
		CInputDevice()
		{
			m_update_info.time_stamp       = 0;
			m_update_info.is_touched       = false;
			m_update_info.touch_pos.x      = 0;
			m_update_info.touch_pos.y      = 0;
			m_update_info.key_down_flag    = 0;
			m_update_info.key_pressed_flag = 0;
		}
		virtual ~CInputDevice()
		{
		}

		virtual const InputUpdateInfo& update(void)
		{
			m_update_info.key_pressed_flag = s_GetButtons();
			m_update_info.is_touched       = s_is_pressed;
			m_update_info.touch_pos.x      = s_pressed_x;
			m_update_info.touch_pos.y      = s_pressed_y;

			return m_update_info;
		}
		virtual const InputUpdateInfo& getUpdateInfo(void) const
		{
			return m_update_info;
		}
		virtual bool  checkKeyDown(KEY key)
		{
			return false;
		}
		virtual bool  checkKeyPressed(KEY key)
		{
			return ((m_update_info.key_pressed_flag & key) != 0);
		}

	private:
		InputUpdateInfo m_update_info;

	};
}

target::InputDevice* target::getInputDevice(void)
{
	static target::CInputDevice s_input_device;

	return &s_input_device;
}
