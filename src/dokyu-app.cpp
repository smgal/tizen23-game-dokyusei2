
#include <tizen.h>
#include <Evas.h>
#include <Ecore.h>
#include <cairo.h>
#include <app.h>
#include <system_info.h>

#include <assert.h>
#include <string.h>

#include "common-appfw.h"

///////////////////////////////////////

extern "C" void app_init(Evas_Object* parent);
extern "C" void app_done(void);

///////////////////////////////////////

namespace
{
	int _getScreenWidth(void)
	{
		int value;
		int ret = system_info_get_platform_int("http://tizen.org/feature/screen.width", &value);

		return (ret == SYSTEM_INFO_ERROR_NONE) ? value : 0;
	}

	int _getScreenHeight(void)
	{
		int value;
		int ret = system_info_get_platform_int("http://tizen.org/feature/screen.height", &value);

		return (ret == SYSTEM_INFO_ERROR_NONE) ? value : 0;
	}
}

///////////////////////////////////////

typedef struct _AppData
{
	Ecore_Timer* timer;
	Evas_Object* image_obj;
	AppFw::BufferDesc buffer_desc;

	struct Scale
	{
		unsigned int check_flag;
		double ratio;
		double offset_x;
		double offset_y;

		Scale()
		: check_flag(0)
		, ratio(1.0)
		, offset_x(0.0)
		, offset_y(0.0)
		{}

		inline static unsigned long makeCheckFlag(int w, int h)
		{
			typedef unsigned long UL32;
			return UL32(w & 0xFFFF) << 16 | UL32(h & 0xFFFF);
		}

		void update(int dst_w, int dst_h, int src_w, int src_h)
		{
			unsigned int new_check_flag = makeCheckFlag(dst_w, dst_h);
			if (check_flag != new_check_flag)
			{
				double scale_x = dst_w * 1.0 / src_w;
				double scale_y = dst_h * 1.0 / src_h;

				if (scale_x > scale_y)
				{
					ratio = scale_y;
					offset_x = (double(dst_w) / ratio - src_w) / 2;
					offset_y = 0.0;
				}
				else
				{
					ratio = scale_x;
					offset_x = 0.0;
					offset_y = (double(dst_h) / ratio - src_h) / 2;
				}

				check_flag = new_check_flag;
			}
		}
	} scale;
} AppData;

static AppData s_app_data = { NULL, NULL };
static AppFw* s_ref_app_fw = NULL;

///////////////////////////////////////

static void _clear_timer(AppData* p_ad);
static void _set_timer(AppData* p_ad);
static void _on_pixel_cb(void *data, Evas_Object *o);
static void _on_mouse_down(void *data, Evas *e, Evas_Object *eo, void *event);
static void _on_mouse_up(void *data, Evas *e, Evas_Object *eo, void *event);
static void _on_mouse_move(void *data, Evas *e, Evas_Object *eo, void *event);

///////////////////////////////////////

void app_init(Evas_Object* parent)
{
	Evas_Object* ret_image_main = NULL;

	// create evas image object
	{
		Evas_Object* p_win = (Evas_Object*)parent;

		Evas_Coord parent_x = 0;
		Evas_Coord parent_y = 0;
		Evas_Coord parent_width = _getScreenHeight();
		Evas_Coord parent_height = _getScreenWidth();
	
		ret_image_main = evas_object_image_filled_add(evas_object_evas_get(p_win));
		evas_object_image_size_set(ret_image_main, parent_width, parent_height);
		evas_object_image_alpha_set(ret_image_main, EINA_TRUE);
		evas_object_image_colorspace_set(ret_image_main, EVAS_COLORSPACE_ARGB8888);
		evas_object_move(ret_image_main, parent_x, parent_y);
		evas_object_resize(ret_image_main, parent_width, parent_height);

		evas_object_image_pixels_get_callback_set(ret_image_main, _on_pixel_cb, (void*)&s_app_data);
		evas_object_event_callback_add(ret_image_main, EVAS_CALLBACK_MOUSE_DOWN, _on_mouse_down, NULL);
		evas_object_event_callback_add(ret_image_main, EVAS_CALLBACK_MOUSE_UP, _on_mouse_up, NULL);
		evas_object_event_callback_add(ret_image_main, EVAS_CALLBACK_MOUSE_MOVE, _on_mouse_move, NULL);

		evas_object_show(ret_image_main);
	}

	// create app data
	{
		s_app_data.image_obj = ret_image_main;

		s_app_data.buffer_desc.width = 640;
		s_app_data.buffer_desc.height = 400;
		s_app_data.buffer_desc.bytes_per_line = s_app_data.buffer_desc.width * 4;
		s_app_data.buffer_desc.bits_per_pixel = 32;
		s_app_data.buffer_desc.p_start_address = new unsigned char[s_app_data.buffer_desc.bytes_per_line * s_app_data.buffer_desc.height];
		s_app_data.buffer_desc.native_handle = 0;
	}

	// create game framework
	{
		s_ref_app_fw = AppFw::create();
		assert(s_ref_app_fw);

		{
			AppFw::AppDesc app_desc;

			char* res_path_buff = app_get_resource_path();
			strcpy(app_desc.res_path, res_path_buff);
			free(res_path_buff);

			s_ref_app_fw->onCreate(app_desc);
		}
	}

	_set_timer(&s_app_data);
}

void app_done(void)
{
	// remove timer
	{
		_clear_timer(&s_app_data);
	}

	// remove game framework
	if (s_ref_app_fw)
	{
		s_ref_app_fw->onDestroy();
		delete s_ref_app_fw;
		s_ref_app_fw = NULL;
	}

	// remove app data
	{
		delete[] (unsigned char*)s_app_data.buffer_desc.p_start_address;
		s_app_data.buffer_desc.p_start_address = NULL;

		if (s_app_data.image_obj)
		{
			evas_object_del(s_app_data.image_obj);
			s_app_data.image_obj = NULL;
		}

		s_app_data.timer = NULL;
	}
}

static Eina_Bool _on_timer(void* data)
{
	AppData* p_ad = (AppData*)data;

	if (p_ad == NULL)
		return EINA_FALSE;

	_set_timer(p_ad);

	return ECORE_CALLBACK_RENEW;
}

void _set_timer(AppData* p_ad)
{
	if (p_ad)
	{
		_clear_timer(p_ad);

		p_ad->timer = ecore_timer_add(0.005, _on_timer, (const void*)p_ad);

		evas_object_image_pixels_dirty_set(p_ad->image_obj, EINA_TRUE);
	}
}

void _clear_timer(AppData* p_ad)
{
	if (p_ad && p_ad->timer)
	{
		ecore_timer_del(p_ad->timer);
		p_ad->timer = NULL;
	}
}

void _on_pixel_cb(void *data, Evas_Object *o)
{
	AppData* p_ad = (AppData*)data;

	assert(p_ad);

	// update game buffer
	s_ref_app_fw->onProcess(p_ad->buffer_desc);

	// update evas buffer
	{
		Evas_Object* obj = p_ad->image_obj;

		int width, height;
		evas_object_image_size_get (obj, &width, &height);

		unsigned char* buffer = (unsigned char*)evas_object_image_data_get (obj, 1);
		int stride = evas_object_image_stride_get (obj);

		AppFw::BufferDesc& buffer_desc = s_app_data.buffer_desc;

		cairo_surface_t *dst_surface = cairo_image_surface_create_for_data (buffer, CAIRO_FORMAT_ARGB32, width, height, stride);
		cairo_t *dst_cr = cairo_create (dst_surface);

		{
			cairo_surface_t *src_surface = cairo_image_surface_create_for_data ((unsigned char*)buffer_desc.p_start_address, CAIRO_FORMAT_ARGB32, buffer_desc.width, buffer_desc.height, buffer_desc.bytes_per_line);

			// center alignment
			{
				AppData::Scale& scale = p_ad->scale;
				scale.update(width, height, buffer_desc.width, buffer_desc.height);

				cairo_scale(dst_cr, scale.ratio, scale.ratio);
				cairo_set_source_surface(dst_cr, src_surface, scale.offset_x, scale.offset_y);
				cairo_paint(dst_cr);
			}

			cairo_surface_destroy(src_surface);
		}

		cairo_surface_flush(dst_surface);

		evas_object_image_data_set(obj, cairo_image_surface_get_data(dst_surface));
		evas_object_image_data_update_add(obj, 0, 0, width, height);

		cairo_destroy(dst_cr);
		cairo_surface_destroy(dst_surface);
	}
}

void _on_mouse_down(void *data, Evas *e, Evas_Object *eo, void *event)
{
	Evas_Event_Mouse_Down* ev = (Evas_Event_Mouse_Down*)event;

	int touch_x = ev->canvas.x;
	int touch_y = ev->canvas.y;

	s_ref_app_fw->onTouchDown(touch_x, touch_y);
}

void _on_mouse_up(void *data, Evas *e, Evas_Object *eo, void *event)
{
	Evas_Event_Mouse_Up* ev = (Evas_Event_Mouse_Up*)event;

	int touch_x = ev->canvas.x;
	int touch_y = ev->canvas.y;

	s_ref_app_fw->onTouchUp(touch_x, touch_y);
}

void _on_mouse_move(void *data, Evas *e, Evas_Object *eo, void *event)
{
	Evas_Event_Mouse_Move* ev = (Evas_Event_Mouse_Move*)event;

	int touch_x = ev->cur.canvas.x;
	int touch_y = ev->cur.canvas.y;

	s_ref_app_fw->onTouchMove(touch_x, touch_y);
}
