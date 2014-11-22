
#ifndef __TARGET_DEP_H__
#define __TARGET_DEP_H__

namespace target
{
	////////////////////////////////////////////////////////////////////////////
	// Input

	enum KEY
	{
		KEY_A        = 0x0001,
		KEY_B        = 0x0002,
		KEY_X        = 0x0004,
		KEY_Y        = 0x0008,
		KEY_L        = 0x0010,
		KEY_R        = 0x0020,
		KEY_MENU     = 0x0040,
		KEY_SELECT   = 0x0080,
		KEY_LEFT     = 0x0100,
		KEY_RIGHT    = 0x0200,
		KEY_UP       = 0x0400,
		KEY_DOWN     = 0x0800,
		KEY_VOLUP    = 0x1000,
		KEY_VOLDOWN  = 0x2000,
	};

	struct InputUpdateInfo
	{
		unsigned long        time_stamp;
		bool                 is_touched;
		struct { int x, y; } touch_pos;
		unsigned long        key_down_flag;
		unsigned long        key_pressed_flag;
	};

	class InputDevice
	{
	public:
		virtual ~InputDevice()
		{
		}

		virtual  const InputUpdateInfo& update(void) = 0;
		virtual  const InputUpdateInfo& getUpdateInfo(void) const = 0;
		virtual  bool  checkKeyDown(KEY key) = 0;
		virtual  bool  checkKeyPressed(KEY key) = 0;
	};

	InputDevice* getInputDevice(void);

	////////////////////////////////////////////////////////////////////////////
	// File I/O

	namespace file_io
	{
		void setResourcePath(const char* sz_path);

		class StreamRead
		{ 
		public:
			enum SEEK
			{
				SEEK_BEGIN,
				SEEK_CURRENT,
				SEEK_END
			};

			virtual ~StreamRead(void)
			{
			}
				
			virtual long  read(void* p_buffer, long count) = 0;
			virtual long  seek(SEEK origin, long offset) = 0;
			virtual long  getSize(void) = 0;
			virtual void* getPointer(void) = 0;
			virtual bool  isValidPos(void) = 0;
		};

		class StreamReadFile: public StreamRead
		{
		public:
			StreamReadFile(const char* sz_file_name);
			~StreamReadFile(void);

			long  read(void* p_buffer, long count);
			long  seek(SEEK origin, long offset);
			long  getSize(void);
			void* getPointer(void) { return 0; };
			bool  isValidPos(void);

		private:
			struct Impl;
			Impl* p_impl;
		};
	}

	////////////////////////////////////////////////////////////////////////////
	// System

	namespace system
	{
		unsigned long getTicks(void);
	}

	////////////////////////////////////////////////////////////////////////////
	// Screen

	int getScreenWidth(void);
	int getScreenHeight(void);
}

#endif
