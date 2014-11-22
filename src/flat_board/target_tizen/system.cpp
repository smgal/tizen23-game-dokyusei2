
#include "../target_dep.h"

#include <system_info.h>

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

int target::getScreenWidth(void)
{
	// consider of orientation
	return _getScreenHeight();
}

int target::getScreenHeight(void)
{
	// consider of orientation
	return _getScreenWidth();
}
