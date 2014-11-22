
#include "common-appfw.h"
#include "flat_board/target_dep.h"

#include <string>

namespace target
{
	void setTouchPos(int x, int y);
}

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

class AppFwDokyu: public AppFw
{
public:
	struct TouchInfo
	{
		bool is_pressed;
		int x, y;

		TouchInfo()
		: is_pressed(false)
		, x(0)
		, y(0)
		{
		}
	} m_touch_info;

	virtual void onCreate(const AppDesc& app_desc)
	{
		target::file_io::setResourcePath(app_desc.res_path);

		dokyu::init();
	}
	virtual void onDestroy(void)
	{
		dokyu::done();
	}
	virtual void onProcess(const BufferDesc& buffer_desc)
	{
		dokyu::BufferDesc desc =
		{
			buffer_desc.p_start_address,
			buffer_desc.width,
			buffer_desc.height,
			buffer_desc.bytes_per_line,
			buffer_desc.bits_per_pixel
		};

		dokyu::loop(desc);
	}

	virtual void onTouchDown(int x, int y)
	{
		m_touch_info.is_pressed = true;
		m_touch_info.x = x;
		m_touch_info.y = y;

		target::setTouchPos(x, y);
	}
	virtual void onTouchUp(int x, int y)
	{
		m_touch_info.is_pressed = false;

		target::setTouchPos(-1, -1);
	}
	virtual void onTouchMove(int x, int y)
	{
		m_touch_info.is_pressed = true;
		m_touch_info.x = x;
		m_touch_info.y = y;

		target::setTouchPos(x, y);
	}
};

AppFw* AppFw::create(void)
{
	static AppFwDokyu s_app_fw_instance;

	return &s_app_fw_instance;
}
