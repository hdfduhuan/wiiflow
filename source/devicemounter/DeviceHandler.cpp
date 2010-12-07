/****************************************************************************
 * Copyright (C) 2010
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 *
 * for WiiXplorer 2010
 ***************************************************************************/
#include <malloc.h>
#include <unistd.h>
#include <string.h>
#include <ogc/mutex.h>
#include <ogc/system.h>
#include <sdcard/wiisd_io.h>

#include "DeviceHandler.hpp"
#include "wbfs.h"

DeviceHandler * DeviceHandler::instance = NULL;

DeviceHandler::~DeviceHandler()
{
    UnMountAll();
}

DeviceHandler * DeviceHandler::Instance()
{
	if (instance == NULL)
	{
		instance = new DeviceHandler();
	}
	return instance;
}

void DeviceHandler::DestroyInstance()
{
    if(instance)
    {
        delete instance;
    }
    instance = NULL;
}

bool DeviceHandler::MountAll()
{
    bool result = false;

    for(u32 i = SD; i <= USB8; i++)
    {
        if(Mount(i))
            result = true;
    }

    return result;
}

void DeviceHandler::UnMountAll()
{
    for(u32 i = SD; i <= USB8; i++)
        UnMount(i);

    if(sd) delete sd;
    if(usb) delete usb;

    sd = NULL;
    usb = NULL;
}

bool DeviceHandler::Mount(int dev)
{
    if(dev == SD) return MountSD();
    else if(dev >= USB1 && dev <= USB8)
        return MountUSB(dev-USB1);

    return false;
}

bool DeviceHandler::IsInserted(int dev)
{
    if(dev == SD) return SD_Inserted() && sd->IsMounted(0);

    else if(dev >= USB1 && dev <= USB8)
        return USB_Inserted() && usb->IsMounted(dev-USB1);

    return false;
}

void DeviceHandler::UnMount(int dev)
{
    if(dev == SD) UnMountSD();

    else if(dev >= USB1 && dev <= USB8)
        UnMountUSB(dev-USB1);
}

bool DeviceHandler::MountSD()
{
    if(!sd) sd = new PartitionHandle(&__io_wiisd);

    if(sd->GetPartitionCount() < 1)
    {
        delete sd;
        sd = NULL;
        return false;
    }

    //! Mount only one SD Partition
    return sd->Mount(0, DeviceName[SD]);
}

bool DeviceHandler::MountUSB(int pos)
{
    if(!usb) usb = new PartitionHandle(&__io_usbstorage);

    if(usb->GetPartitionCount() < 1)
    {
        delete usb;
        usb = NULL;
        return false;
    }

    if(pos >= usb->GetPartitionCount())
        return false;

    return usb->Mount(pos, DeviceName[USB1+pos]);
}

bool DeviceHandler::MountAllUSB()
{
    if(!usb) usb = new PartitionHandle(&__io_usbstorage);

    bool result = false;

    for(int i = 0; i < usb->GetPartitionCount(); i++)
        if(MountUSB(i)) result = true;

    return result;
}

void DeviceHandler::UnMountUSB(int pos)
{
    if(!usb) return;

    if(pos >= usb->GetPartitionCount())
        return;

    usb->UnMount(pos);
}

void DeviceHandler::UnMountAllUSB()
{
    if(!usb) return;

    for(int i = 0; i < usb->GetPartitionCount(); i++)
        usb->UnMount(i);

    delete usb;
    usb = NULL;
}

int DeviceHandler::PathToDriveType(const char * path)
{
    if(!path) return -1;

    for(int i = SD; i <= USB8; i++)
        if(strncmp(path, DeviceName[i], strlen(DeviceName[i])) == 0)
            return i;

    return -1;
}

const char * DeviceHandler::GetFSName(int dev)
{
    if(dev == SD && DeviceHandler::instance->sd)
        return DeviceHandler::instance->sd->GetFSName(0);
    else if(dev >= USB1 && dev <= USB8 && DeviceHandler::instance->usb)
        return DeviceHandler::instance->usb->GetFSName(dev-USB1);

    return NULL;
}

int DeviceHandler::GetFSType(int dev)
{
	const char *name = GetFSName(dev);
	if (strncasecmp(name, "WBFS", 4) == 0)
		return PART_FS_WBFS;
	else if (strncasecmp(name, "FAT", 3) == 0)
		return PART_FS_FAT;
	else if (strncasecmp(name, "NTFS", 4) == 0)
		return PART_FS_NTFS;
	else if (strncasecmp(name, "LINUX", 5) == 0)
		return PART_FS_EXT;
	else
		return -1;
}

s32 DeviceHandler::Open_WBFS(int dev)
{
	u32 part_idx, part_lba, part_size;
	u32 part_fs = GetFSType(dev);

	char *partition = (char *)DeviceName[dev];
	if(dev == SD)
	{
		part_idx = 1;
		part_lba = Instance()->sd->GetLBAStart(dev);

		if(part_fs == PART_FS_WBFS)
			part_size = Instance()->sd->GetSecCount(dev);
		else
			part_size = Instance()->sd->GetSize(dev);
	}
	else
	{
		part_idx = dev;
		part_lba = Instance()->usb->GetLBAStart(dev - USB1);

		if(part_fs == PART_FS_WBFS)
			part_size = Instance()->usb->GetSecCount(dev - USB1);
		else
			part_size = Instance()->usb->GetSize(dev - USB1);
	}

	return WBFS_OpenPart(part_fs, part_idx, part_lba, part_size, partition);	
}