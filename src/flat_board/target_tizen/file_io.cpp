
#include "../target_dep.h"

static const int StreamRead_SEEK_END = target::file_io::StreamRead::SEEK_END;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//extern "C" void g_printLog(const char* sz_log);
#define g_printLog(s)

namespace
{
	template <typename T>
	T getMin(T a, T b)
	{
		return (a < b) ? a : b;
	}
}

namespace target
{
	namespace file_io
	{
		const char* s_app_path1 = "";
		char s_app_path2[256] = {0, };

		void setResourcePath(const char* sz_path)
		{
			strncpy(s_app_path2, sz_path, 256);
		}

		struct StreamReadFile::Impl
		{
			unsigned char* m_p_buffer;
			unsigned int m_buffer_length;
			unsigned int m_offset;
			bool  m_is_available;

			Impl()
				: m_p_buffer(0)
				, m_buffer_length(0)
				, m_offset(0)
				, m_is_available(false)
			{
			}

			~Impl()
			{
				delete[] m_p_buffer;
			}
		};

		StreamReadFile::StreamReadFile(const char* sz_file_name)
			: p_impl(new Impl)
		{
			char file_name[256];

			strncpy(file_name, s_app_path1, 256);
			strncat(file_name, s_app_path2, 256);
			strncat(file_name, sz_file_name, 256);

			{
				FILE* p_file = fopen(file_name, "rb");
				if (p_file)
				{
					fseek(p_file, 0, SEEK_END);
					p_impl->m_buffer_length = ftell(p_file);
					fseek(p_file, 0, SEEK_SET);
					p_impl->m_p_buffer = new unsigned char[p_impl->m_buffer_length];

					fread(p_impl->m_p_buffer, p_impl->m_buffer_length, 1, p_file);

					fclose(p_file);
				}
			}

			p_impl->m_offset = 0;
			p_impl->m_is_available = (p_impl->m_p_buffer != 0);

			if (p_impl->m_p_buffer)
			{
				char sz_log[256];
				sprintf(sz_log, "[SMGAL] %s found!", file_name);
				g_printLog(sz_log);
			}
			else
			{
				char sz_log[256];
				sprintf(sz_log, "[SMGAL] %s NOT found!", file_name);
				g_printLog(sz_log);
			}
		}

		StreamReadFile::~StreamReadFile(void)
		{
			delete p_impl;
		}

		long  StreamReadFile::read(void* p_buffer, long count)
		{
			if (!p_impl->m_is_available)
				return 0;

			long copied_bytes = getMin(count, long(p_impl->m_buffer_length - p_impl->m_offset));

			if (copied_bytes <= 0)
				return 0;

			memcpy(p_buffer, &p_impl->m_p_buffer[p_impl->m_offset], copied_bytes);

			p_impl->m_offset += copied_bytes;

			return copied_bytes;
		}

		long  StreamReadFile::seek(SEEK origin, long offset)
		{
			if (!p_impl->m_is_available)
				return -1;

			switch (origin)
			{
			case StreamRead::SEEK_BEGIN:
				p_impl->m_offset = offset;
				break;
			case StreamRead::SEEK_CURRENT:
				p_impl->m_offset += offset;
				break;
			case StreamRead_SEEK_END:
				p_impl->m_offset = p_impl->m_buffer_length + offset;
				break;
			default:
				return -1;
			}

			return p_impl->m_offset;
		}

		long  StreamReadFile::getSize(void)
		{
			if (!p_impl->m_is_available)
				return -1;

			return p_impl->m_buffer_length;
		}

		bool  StreamReadFile::isValidPos(void)
		{
			if (!p_impl->m_is_available)
				return false;

			return (p_impl->m_offset < p_impl->m_buffer_length);
		}
	}
}
