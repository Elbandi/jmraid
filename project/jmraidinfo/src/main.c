#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdarg.h>
#include <getopt.h>
#include <json-c/json.h>

#include <jmraid.h>

int g_print_json = 0;
int g_print_indent = 0;

const char *get_raid_state_text(uint8_t raid_state)
{
	switch (raid_state)
	{
		case 0x00: return "Broken";
		case 0x01: return "Degraded";
		case 0x02: return "Rebuilding";
		case 0x03: return "Normal";
		case 0x04: return "Expansion";
		case 0x05: return "Backup";
		default: return "?";
	}
}

const char *get_raid_level_text(uint8_t raid_level)
{
	switch (raid_level)
	{
		case 0x00: return "RAID 0";
		case 0x01: return "RAID 1";
		case 0x02: return "JBOD / LARGE";
		case 0x03: return "RAID 3";
		case 0x04: return "CLONE";
		case 0x05: return "RAID 5";
		case 0x06: return "RAID 10";
		default: return "?";
	}
}

const char *get_raid_rebuild_priority_text(uint16_t raid_rebuild_priority)
{
	// 0x4000 = low, 0x2000 = low-middle, 0x1000 = middle, 0x0800 = middle-high, 0x0400 = high
	if (raid_rebuild_priority <= ((0x0400 + 0x0800) / 2))
	{
		return "Highest";
	}
	else if (raid_rebuild_priority <= ((0x0800 + 0x1000) / 2))
	{
		return "High";
	}
	else if (raid_rebuild_priority <= ((0x1000 + 0x2000) / 2))
	{
		return "Medium";
	}
	else if (raid_rebuild_priority <= ((0x2000 + 0x4000) / 2))
	{
		return "Low";
	}
	else
	{
		return "Lowest";
	}
}

const char *get_sata_port_type_text(uint8_t sata_port_type)
{
	switch (sata_port_type)
	{
		case 0x00: return "No Device";
		case 0x01: return "Hard Disk";
		case 0x02: return "RAID Disk";
		case 0x03: return "Optical Drive";
		case 0x04: return "Bad Port";
		case 0x05: return "Skip";
		case 0x06: return "Off";
		case 0x07: return "Host";
		default: return "?";
	}
}

const char *get_sata_port_speed_text(uint8_t sata_port_speed)
{
	switch (sata_port_speed)
	{
		case 0x00: return "No Connection";
		case 0x01: return "Gen 1";
		case 0x02: return "Gen 2";
		case 0x03: return "Gen 3";
		default: return "?";
	}
}

const char *get_sata_page_state_text(uint8_t sata_page_state)
{
	switch (sata_page_state)
	{
		case 0x00: return "Null";
		case 0x01: return "Valid";
		case 0x02: return "Hooked to PM";
		case 0x03: return "Spare Drive";
		case 0x04: return "Bad Page";
		default: return "?";
	}
}

const char *get_smart_attribute_name(uint8_t id)
{
	switch (id)
	{
		case 0x01: return "Raw Read Error Rate";
		case 0x02: return "Throughput Performance";
		case 0x03: return "Spin Up Time";
		case 0x04: return "Start/Stop Count";
		case 0x05: return "Reallocated Sectors Count";
		case 0x06: return "Read Channel Margin";
		case 0x07: return "Seek Error Rate";
		case 0x08: return "Seek Time Performance";
		case 0x09: return "Power-On Time Count";
		case 0x0A: return "Spin Retry Count";
		case 0x0B: return "Drive Calibration Retry Count";
		case 0x0C: return "Drive Power Cycle Count";
		case 0x0D: return "Soft Read Error Rate";
		case 0xB7: return "SATA Downshift Count";
		case 0xC0: return "Power off Retract Cycle";
		case 0xC1: return "Load/Unload Cycle Count";
		case 0xC2: return "HDD Temperature";
		case 0xC3: return "Hardware ECC Recovered";
		case 0xC4: return "Reallocation Event Count";
		case 0xC5: return "Current Pending Sector Count";
		case 0xC6: return "Off-Line Uncorrectable Sector Count";
		case 0xC7: return "Ultra ATA CRC Error Count";
		case 0xC8: return "Write Error Rate";
		default: return "?";
	}
}

void print(const char* format, ...)
{
	va_list arglist;
	int len = g_print_indent * 2;
	while (len-- > 0)
	{
		putchar(' ');
	}
	va_start(arglist, format);
	vprintf(format, arglist);
	va_end(arglist);
}

void debug_print(const char* format, ...)
{
	va_list arglist;
	int len = g_print_indent * 2;
	while (len-- > 0)
	{
		putchar(' ');
	}
	printf("[DEBUG] ");
	va_start(arglist, format);
	vprintf(format, arglist);
	va_end(arglist);
}

void print_buffer(const void *data, uint32_t size, bool print_addr, bool print_text)
{
	const uint8_t *d = (const uint8_t *)data;
	uint32_t i;
	for (i = 0; i < size; i += 16)
	{
		uint32_t j;
		int len = g_print_indent * 2;
		while (len-- > 0)
		{
			putchar(' ');
		}
		if (print_addr)
		{
			printf("%08X | ", i);
		}
		for (j = i; j < i + 16; j++)
		{
			if (j > i)
			{
				putchar(' ');
			}
			if (j < size)
			{
				printf("%02X", d[j]);
			}
			else
			{
				printf("  ");
			}
		}
		if (print_text)
		{
			printf(" | ");
			for (j = i; j < i + 16; j++)
			{
				if (j < size)
				{
					putchar((d[j] >= 32) && (d[j] < 128) ? d[j] : '.');
				}
				else
				{
					putchar(' ');
				}
			}
		}
		printf("\n");
	}
}

void print_chip_info(const struct jmraid_chip_info *info)
{
	print("Firmware version = %02d.%02d.%02d.%02d\n", info->firmware_version[3], info->firmware_version[2], info->firmware_version[1], info->firmware_version[0]);
	print("Manufacturer     = %s\n", info->manufacturer);
	print("Product name     = %s\n", info->product_name);
	print("Serial number    = %d\n", info->serial_number);
}

void add_chip_info(json_object* parent, const struct jmraid_chip_info* info)
{
	json_object* obj = json_object_new_object();
	char str[64];

	sprintf(str, "%02d.%02d.%02d.%02d", info->firmware_version[3], info->firmware_version[2], info->firmware_version[1], info->firmware_version[0]);
	json_object_object_add(obj, "firmware_version", json_object_new_string(str));
	json_object_object_add(obj, "manufacturer", json_object_new_string(info->manufacturer));
	json_object_object_add(obj, "product_name", json_object_new_string(info->product_name));
	json_object_object_add(obj, "serial_number", json_object_new_int64(info->serial_number));
	json_object_object_add(parent, "chip_info", obj);
}


void print_sata_info(const struct jmraid_sata_info *info)
{
	int i;
	for (i = 0; i < 5; i++)
	{
		const struct jmraid_sata_info_item *item = &info->item[i];
		if (i > 0)
		{
			print("\n");
		}
		print("SATA Port %d\n", i);
		print("\n");
		g_print_indent++;
		if ((item->port_type == 0x01) || (item->port_type == 0x02))
		{
			print("Model name        = %s\n", item->model_name);
			print("Serial number     = %s\n", item->serial_number);
			print("Capacity          = %.2f GB\n", (float)item->capacity / (1 * 1024 * 1024 * 1024));
			print("Port type         = %d (%s)\n", item->port_type, get_sata_port_type_text(item->port_type));
			print("Port speed        = %d (%s)\n", item->port_speed, get_sata_port_speed_text(item->port_speed));
			print("Page 0 state      = %d (%s)\n", item->page_0_state, get_sata_page_state_text(item->page_0_state));
			print("RAID index        = %d\n", item->page_0_raid_index);
			print("RAID member index = %d\n", item->page_0_raid_member_index);
			print("Port              = %d\n", item->port);
		}
		else
		{
			print("Port type = %d (%s)\n", item->port_type, get_sata_port_type_text(item->port_type));
		}
		g_print_indent--;
	}
}

void add_sata_info(json_object* parent, const struct jmraid_sata_info* info)
{
	int i;
	json_object* arr = json_object_new_array();
	for (i = 0; i < 5; i++)
	{
		const struct jmraid_sata_info_item* item = &info->item[i];
		json_object* obj = json_object_new_object();
		json_object_object_add(obj, "port", json_object_new_int(item->port));
		json_object_object_add(obj, "type", json_object_new_int(item->port_type));
		json_object_object_add(obj, "type_str", json_object_new_string(get_sata_port_type_text(item->port_type)));
		if ((item->port_type == 0x01) || (item->port_type == 0x02))
		{
			json_object_object_add(obj, "model_name", json_object_new_string(item->model_name));
			json_object_object_add(obj, "serial_number", json_object_new_string(item->serial_number));
			json_object_object_add(obj, "capacity", json_object_new_int64(item->capacity));
			json_object_object_add(obj, "port_speed", json_object_new_int(item->port_speed));
			json_object_object_add(obj, "port_speed_str", json_object_new_string(get_sata_port_speed_text(item->port_speed)));
			json_object_object_add(obj, "page_0_state", json_object_new_int(item->page_0_state));
			json_object_object_add(obj, "page_0_state_str", json_object_new_string(get_sata_page_state_text(item->page_0_state)));
			json_object_object_add(obj, "raid_index", json_object_new_int(item->page_0_raid_index));
			json_object_object_add(obj, "raid_member_index", json_object_new_int(item->page_0_raid_member_index));
		}
		json_object_array_add(arr, obj);
	}
	json_object_object_add(parent, "sata_info", arr);
}


void print_sata_port_info(const struct jmraid_sata_port_info *info)
{
	if ((info->port_type == 0x01) || (info->port_type == 0x02))
	{
		print("Model name        = %s\n", info->model_name);
		print("Serial number     = %s\n", info->serial_number);
		print("Firmware version  = %s\n", info->firmware_version);
		print("Capacity          = %.2f GB\n", (float)info->capacity / (1 * 1024 * 1024 * 1024));
		print("Capacity used     = %.2f GB\n", (float)info->capacity_used / (1 * 1024 * 1024 * 1024));
		print("Port type         = %d (%s)\n", info->port_type, get_sata_port_type_text(info->port_type));
		print("Port              = %d\n", info->port);
		print("Page 0 state      = %d (%s)\n", info->page_0_state, get_sata_page_state_text(info->page_0_state));
		print("RAID index        = %d\n", info->page_0_raid_index);
		print("RAID member index = %d\n", info->page_0_raid_member_index);
	}
	else
	{
		print("Port type = %d (%s)\n", info->port_type, get_sata_port_type_text(info->port_type));
	}
}

void add_sata_port_info(json_object* parent, const struct jmraid_sata_port_info* info)
{
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "port", json_object_new_int(info->port));
	json_object_object_add(obj, "type", json_object_new_int(info->port_type));
	json_object_object_add(obj, "type_str", json_object_new_string(get_sata_port_type_text(info->port_type)));
	if ((info->port_type == 0x01) || (info->port_type == 0x02))
	{
		json_object_object_add(obj, "model_name", json_object_new_string(info->model_name));
		json_object_object_add(obj, "serial_number", json_object_new_string(info->serial_number));
		json_object_object_add(obj, "firmware_version", json_object_new_string(info->firmware_version));
		json_object_object_add(obj, "capacity", json_object_new_int64(info->capacity));
		json_object_object_add(obj, "capacity_used", json_object_new_int64(info->capacity_used));
		json_object_object_add(obj, "page_0_state", json_object_new_int(info->page_0_state));
		json_object_object_add(obj, "page_0_state_str", json_object_new_string(get_sata_page_state_text(info->page_0_state)));
		json_object_object_add(obj, "raid_index", json_object_new_int(info->page_0_raid_index));
		json_object_object_add(obj, "raid_member_index", json_object_new_int(info->page_0_raid_member_index));
	}
	json_object_object_add(parent, "sata_port_info", obj);
}

void print_raid_port_info(const struct jmraid_raid_port_info *info)
{
	if (info->port_state != 0x00)
	{
		int i;
		print("Model name       = %s\n", info->model_name);
		print("Serial number    = %s\n", info->serial_number);
		print("Port state       = %d \n", info->port_state);
		print("Level            = %d (%s)\n", info->level, get_raid_level_text(info->level));
		print("Capacity         = %.2f GB\n", (float)info->capacity / (1 * 1024 * 1024 * 1024));
		print("State            = %d (%s)\n", info->state, get_raid_state_text(info->state));
		print("Member count     = %d\n", info->member_count);
		print("Rebuild priority = %d (%s)\n", info->rebuild_priority, get_raid_rebuild_priority_text(info->rebuild_priority));
		print("Standby timer    = %d sec\n", info->standby_timer);
		print("Password         = %s\n", info->password);
		print("Rebuild progress = %.2f %%\n", info->capacity ? (float)info->rebuild_progress * 100 / info->capacity : 0);
		for (i = 0; i < info->member_count; i++)
		{
			const struct jmraid_raid_port_info_member *member = &info->member[i];
			print("\n");
			print("Member %d\n", i);
			print("\n");
			g_print_indent++;
			print("Ready         = %d\n", member->ready);
			print("LBA48 support = %d\n", member->lba48_support);
			print("SATA port     = %d\n", member->sata_port);
			print("SATA page     = %d\n", member->sata_page);
			print("SATA base     = %d\n", member->sata_base);
			print("SATA size     = %.2f GB\n", (float)member->sata_size / (1 * 1024 * 1024 * 1024));
			g_print_indent--;
		}
	}
	else
	{
		print("Port state = %d \n", info->port_state);
	}
}

void add_raid_port_info(json_object* parent, const struct jmraid_raid_port_info* info)
{
	json_object* obj = json_object_new_object();
	json_object_object_add(obj, "port", json_object_new_int(info->port_state));
	if (info->port_state != 0x00)
	{
		int i;
		json_object* arr = json_object_new_array();
		json_object_object_add(obj, "model_name", json_object_new_string(info->model_name));
		json_object_object_add(obj, "serial_number", json_object_new_string(info->serial_number));
		json_object_object_add(obj, "raid_level", json_object_new_int(info->level));
		json_object_object_add(obj, "raid_level_str", json_object_new_string(get_raid_level_text(info->level)));
		json_object_object_add(obj, "capacity", json_object_new_int64(info->capacity));
		json_object_object_add(obj, "state", json_object_new_int(info->state));
		json_object_object_add(obj, "state_str", json_object_new_string(get_raid_state_text(info->state)));
		json_object_object_add(obj, "member_count", json_object_new_int(info->member_count));
		json_object_object_add(obj, "rebuild_priority", json_object_new_int(info->rebuild_priority));
		json_object_object_add(obj, "rebuild_priority_str", json_object_new_string(get_raid_rebuild_priority_text(info->rebuild_priority)));
		json_object_object_add(obj, "standby_timer", json_object_new_int(info->standby_timer));
		json_object_object_add(obj, "password", json_object_new_string(info->password));
		json_object_object_add(obj, "rebuild_progress", json_object_new_int64(info->rebuild_progress));

		for (i = 0; i < info->member_count; i++)
		{
			const struct jmraid_raid_port_info_member* member = &info->member[i];
			json_object* obj2 = json_object_new_object();
			json_object_object_add(obj2, "id", json_object_new_int(i));
			json_object_object_add(obj2, "ready", json_object_new_int(member->ready));
			json_object_object_add(obj2, "lba48_support", json_object_new_int(member->lba48_support));
			json_object_object_add(obj2, "sata_port", json_object_new_int(member->sata_port));
			json_object_object_add(obj2, "sata_page", json_object_new_int(member->sata_page));
			json_object_object_add(obj2, "sata_base", json_object_new_int(member->sata_base));
			json_object_object_add(obj2, "sata_size", json_object_new_int64(member->sata_size));
			json_object_array_add(arr, obj2);
		}
	}
	json_object_object_add(parent, "raid_port_info", obj);
}

void print_disk_smart_info(const struct jmraid_disk_smart_info *info)
{
	int i;
	for (i = 0; i < 30; i++)
	{
		const struct jmraid_disk_smart_info_attribute *attr = &info->attribute[i];
		if (attr->id != 0)
		{
			print("%3u | %04X | %3u | %3u | %3u | %012llX | %15llu | %s\n", attr->id, attr->flags, attr->threshold, attr->current_value, attr->worst_value, attr->raw_value, attr->raw_value, get_smart_attribute_name(attr->id));
		}
	}
}

void add_disk_smart_info(json_object* parent, const struct jmraid_disk_smart_info* info)
{
	int i;
	json_object* arr = json_object_new_array();
	for (i = 0; i < 30; i++)
	{
		const struct jmraid_disk_smart_info_attribute* attr = &info->attribute[i];
		if (attr->id != 0)
		{
			json_object* obj2 = json_object_new_object();
			json_object_object_add(obj2, "id", json_object_new_int(attr->id));
			json_object_object_add(obj2, "name", json_object_new_string(get_smart_attribute_name(attr->id)));
			json_object_object_add(obj2, "flags", json_object_new_int(attr->flags));
			json_object_object_add(obj2, "threshold", json_object_new_int(attr->threshold));
			json_object_object_add(obj2, "current_value", json_object_new_int(attr->current_value));
			json_object_object_add(obj2, "worst_value", json_object_new_int(attr->worst_value));
			json_object_object_add(obj2, "raw_value", json_object_new_int64(attr->raw_value));
			json_object_array_add(arr, obj2);
		}
	}
	json_object_object_add(parent, "disk_smart_info", arr);
}

void check_disk(json_object* parent, const char *disk_name)
{
	struct jmraid jmraid;

	if (!g_print_json) {
		print("\n");
		print("Check \"%s\" ...\n", disk_name);
	}
	g_print_indent++;
	jmraid_init(&jmraid);
	if (!jmraid_open(&jmraid, disk_name, 0))
	{
		if (g_print_json) {
			fprintf(stderr, "%s\n", "jmraid_open failed");
		}
		else {
			print("\n");
			print("jmraid_open failed\n");
		}
	}
	else
	{
		uint32_t vendor_id;
		if (!jmraid_detect_vendor_id(&jmraid, &vendor_id))
		{
			if (g_print_json) {
				fprintf(stderr, "%s\n", "jmraid_detect_vendor_id failed");
			}
			else {
				print("\n");
				print("jmraid_detect_vendor_id failed\n");
			}
		}
		else
		{
			int i;
			struct jmraid_chip_info chip_info;
			struct jmraid_sata_info sata_info;
			struct jmraid_sata_port_info sata_port_info;
			struct jmraid_raid_port_info raid_port_info;
			struct jmraid_disk_smart_info disk_smart_info;
			bool is_raid_or_spare_disk[5];

			memset(is_raid_or_spare_disk, 0, sizeof(is_raid_or_spare_disk));

			jmraid_set_vendor_id(&jmraid, vendor_id);

			if (!g_print_json) {
				print("\n");
				print("Get chip info ...\n");
				print("\n");
			}
			g_print_indent++;
			if (!jmraid_get_chip_info(&jmraid, &chip_info))
			{
				if (g_print_json) {
					fprintf(stderr, "%s\n", "jmraid_get_chip_info failed");
				}
				else {
					print("jmraid_get_chip_info failed\n");
				}
			}
			else
			{
				if (g_print_json) add_chip_info(parent, &chip_info);
				else print_chip_info(&chip_info);
			}
			g_print_indent--;

			if (!g_print_json) {
				print("\n");
				print("Get SATA info ...\n");
				print("\n");
			}
			g_print_indent++;
			if (!jmraid_get_sata_info(&jmraid, &sata_info))
			{
				if (g_print_json) {
					fprintf(stderr, "%s\n", "jmraid_get_sata_info failed");
				}
				else {
					print("jmraid_get_sata_info failed\n");
				}
			}
			else
			{
				if (g_print_json) add_sata_info(parent, &sata_info);
				else print_sata_info(&sata_info);
				for (i = 0; i < 5; i++)
				{
					is_raid_or_spare_disk[i] = (sata_info.item[i].port_type == 0x02) || ((sata_info.item[i].port_type == 0x01) && (sata_info.item[i].page_0_state == 0x03));
				}
			}
			g_print_indent--;

			for (i = 0; i < 5; i++)
			{
				if (!g_print_json) {
					print("\n");
					print("Get SATA port %d info ...\n", i);
					print("\n");
				}
				g_print_indent++;
				if (!jmraid_get_sata_port_info(&jmraid, i, &sata_port_info))
				{
					if (g_print_json) {
						fprintf(stderr, "%s\n", "jmraid_get_sata_port_info failed");
					}
					else {
						print("jmraid_get_sata_port_info failed\n");
					}
				}
				else
				{
					if (g_print_json) add_sata_port_info(parent, &sata_port_info);
					else print_sata_port_info(&sata_port_info);
				}
				g_print_indent--;
			}

			for (i = 0; i < 5; i++)
			{
				if (!g_print_json) {
					print("\n");
					print("Get RAID port %d info ...\n", i);
					print("\n");
				}
				g_print_indent++;
				if (!jmraid_get_raid_port_info(&jmraid, i, &raid_port_info))
				{
					if (g_print_json) {
						fprintf(stderr, "%s\n", "jmraid_get_raid_port_info failed");
					}
					else {
						print("jmraid_get_raid_port_info failed\n");
					}
				}
				else
				{
					if (g_print_json) add_raid_port_info(parent, &raid_port_info);
					else print_raid_port_info(&raid_port_info);
				}
				g_print_indent--;
			}

			for (i = 0; i < 5; i++)
			{
				if (is_raid_or_spare_disk[i])
				{
					if (!g_print_json) {
						print("\n");
						print("Get SMART info (disk %d) ...\n", i);
						print("\n");
					}
					g_print_indent++;
					if (!jmraid_get_disk_smart_info(&jmraid, i, &disk_smart_info))
					{
						if (g_print_json) {
							fprintf(stderr, "%s\n", "jmraid_get_disk_smart_info failed");
						}
						else {
							print("jmraid_get_disk_smart_info failed\n");
						}
					}
					else
					{
						if (g_print_json) add_disk_smart_info(parent, &disk_smart_info);
						else print_disk_smart_info(&disk_smart_info);
					}
					g_print_indent--;
				}
			}

#if 0
			for (i = 0; i < 5; i++)
			{
				if (is_raid_or_spare_disk[i])
				{
					uint8_t data[512];
					print("\n");
					print("ATA IDENTIFY DEVICE (disk %d) ...\n", i);
					print("\n");
					g_print_indent++;
					if (!jmraid_ata_identify_device(&jmraid, i, data))
					{
						print("jmraid_ata_identify_device failed\n");
					}
					else
					{
						print_buffer(data, sizeof(data), true, true);
					}
					g_print_indent--;
				}
			}
#endif

#if 0
			for (i = 0; i < 5; i++)
			{
				if (is_raid_or_spare_disk[i])
				{
					uint8_t data[512];
					print("\n");
					print("ATA SMART READ DATA (disk %d) ...\n", i);
					print("\n");
					g_print_indent++;
					if (!jmraid_ata_smart_read_data(&jmraid, i, data))
					{
						print("jmraid_ata_smart_read_data failed\n");
					}
					else
					{
						print_buffer(data, sizeof(data), true, true);
					}
					g_print_indent--;
				}
			}
#endif
		}

		if (!jmraid_close(&jmraid))
		{
			if (g_print_json) {
				fprintf(stderr, "%s\n", "jmraid_get_disk_smart_info failed");
			}
			else {
				print("\n");
				print("jmraid_close failed\n");
			}
		}
	}
	g_print_indent--;
}

int main(int argc, char *argv[])
{
	int disk_number;
	int c;
	char disk_name[32];
	json_object* root;
	while ((c = getopt(argc, argv, "j")) != -1) {
		switch (c) {
		case 'j':
			g_print_json = 1;
			break;
		case '?':
			break;
		default:
			fprintf(stderr, "?? getopt returned character code 0%o ??\n", c);
		}
	}
	if (!g_print_json) print("JMicron RAID info\n");

	if (optind < argc) {
		root = json_object_new_object();
		strncpy(disk_name, argv[optind++], 32);
		check_disk(root, disk_name);
	}
	else {
		root = json_object_new_array();
		for (disk_number = 0; disk_number < 16; disk_number++)
		{
			json_object* obj = json_object_new_object();
#ifdef _WIN32
			sprintf(disk_name, "\\\\.\\PhysicalDrive%d", disk_number + 1);
#else
			sprintf(disk_name, "/dev/sd%c", 'a' + disk_number);
#endif
			check_disk(obj, disk_name);
			json_object_array_add(root, obj);
		}
	}
	if (g_print_json) {
		printf("%s\n", json_object_to_json_string(root));
	}
	json_object_put(root);

	return 0;
}
