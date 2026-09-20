#include "JevSettings.h"

UJevSettings* UJevSettings::Get()
{
	return GetMutableDefault<UJevSettings>();
}
