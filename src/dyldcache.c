/**
 * GreenPois0n Absinthe - dyldcache.c
 * Copyright (C) 2010 Chronic-Dev Team
 * Copyright (C) 2010 Joshua Hill
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "debug.h"
#include "common.h"
#include "endianness.h"

#include "dyldmap.h"
#include "dyldimage.h"
#include "dyldcache.h"

/*
 * Dyldcache Functions
 */
dyldcache_t* dyldcache_create() {
	dyldcache_t* cache = (dyldcache_t*) malloc(sizeof(dyldcache_t));
	if (cache) {
		memset(cache, '\0', sizeof(dyldcache_t));
	}
	return cache;
}

dyldcache_t* dyldcache_open(const char* path) {
	int i = 0;
	int err = 0;
	uint32_t count = 0;
	uint32_t offset = 0;
	uint32_t length = 0;
	file_t* file = NULL;
	dyldcache_t* cache = NULL;
	dyldimage_t* image = NULL;
	unsigned char* data = NULL;
	unsigned char* buffer = NULL;

	cache = dyldcache_create();
	if (cache) {
		file = file_create();
		if (file == NULL) {
			error("Unable to allocate memory for file object\n");
			dyldcache_free(cache);
			return NULL;
		}
		err = file_read(path, &buffer, &length);
		if (err < 0) {
			error("Unable to open file at path %s\n", path);
			dyldcache_free(cache);
			return NULL;
		}

		cache->header = dyldcache_header_parse(buffer);
		if (cache->header == NULL) {
			error("Unable to parse dyldcache header\n");
			dyldcache_free(cache);
			return NULL;
		}

		cache->arch = dyldcache_architecture_parse(buffer);
		if (cache->arch == NULL) {
			error("Unable to parse architecture from dyldcache header\n");
			dyldcache_free(cache);
			return NULL;
		}

		count = cache->header->images_count;
		cache->images = (dyldimage_t**) malloc(sizeof(dyldimage_t*) * count);
		if(cache->images == NULL) {
			error("Unable to allocate dyld image structures\n");
			return NULL;
		}

		offset = cache->header->images_offset;
		for (i = 0; i < count; i++) {
			image = dyldimage_parse(buffer, offset);
			if (image == NULL) {
				error("Unable to parse images from dyldcache\n");
				return NULL;
			}
			offset += sizeof(dyldimage_info_t);
			cache->images[i] = image;
			dyldimage_debug(image);
			image = NULL;
		}

		file->data = buffer;
		file->size = length;
		file->offset = 0;
		file->path = strdup(path);

		dyldcache_debug(cache);
	}
	return cache;
}

void dyldcache_free(dyldcache_t* cache) {
	if (cache) {
		if (cache->header) {
			dyldcache_header_free(cache->header);
			cache->header = NULL;
		}
		free(cache);
	}
}

void dyldcache_debug(dyldcache_t* cache) {
	if (cache) {
		debug("Dyldcache:\n");
		if (cache->header) dyldcache_header_debug(cache->header);
		if (cache->images) dyldcache_images_debug(cache);
		if (cache->maps) dyldcache_maps_debug(cache);
	}
}

/*
 * Dyldcache Architecture Functions
 */
architecture_t* dyldcache_architecture_create() {
	architecture_t* arch = (architecture_t*) malloc(sizeof(architecture_t));
	if (arch) {
		memset(arch, '\0', sizeof(architecture_t*));
	}
	return arch;
}

architecture_t* dyldcache_architecture_parse(unsigned char* data) {
	unsigned char* found = NULL;
	architecture_t* arch = dyldcache_architecture_create();
	if (arch) {
		found = strstr(data, DYLDARCH_ARMV6);
		if (found) {
			arch->name = DYLDARCH_ARMV6;
			arch->cpu_type = kArmType;
			arch->cpu_subtype = kArmv6;
			arch->cpu_endian = kLittleEndian;
			return arch;
		}

		found = strstr(data, DYLDARCH_ARMV7);
		if (found) {
			arch->name = DYLDARCH_ARMV7;
			arch->cpu_type = kArmType;
			arch->cpu_subtype = kArmv7;
			arch->cpu_endian = kLittleEndian;
			return arch;
		}

		// TODO: Add other architectures in here. We only need iPhone for now.
	}

	if (found == NULL) {
		error("Unknown architechure encountered! %s\n", data);
	}

	return arch;
}

void dyldcache_architecture_debug(architecture_t* arch) {
	debug("\tArchitecture:\n");
	debug("\t\tname = %s\n", arch->name);
	debug("\t\tcpu_id = %d\n", arch->cpu_type);
	debug("\t\tcpu_sub_id = %d\n", arch->cpu_subtype);
	debug("\t\tcpu_endian = %s\n", arch->cpu_endian == kLittleEndian ? "little endian" : "big endian");
	debug("\n");
}

void dyldcache_architecture_free(architecture_t* arch) {
	if (arch) {
		free(arch);
	}
}

/*
 * Dyldcache Header Functions
 */
dyldcache_header_t* dyldcache_header_create() {
	dyldcache_header_t* header = (dyldcache_header_t*) malloc(sizeof(dyldcache_header_t));
	if (header) {
		memset(header, '\0', sizeof(dyldcache_header_t));
	}
	return header;
}

void dyldcache_header_free(dyldcache_header_t* header) {
	if (header) {
		free(header);
	}
}

dyldcache_header_t* dyldcache_header_parse(unsigned char* data) {
	dyldcache_header_t* header = dyldcache_header_create();
	if (header) {
		memcpy(header, data, sizeof(dyldcache_header_t));
	}
	return header;
}

void dyldcache_header_debug(dyldcache_header_t* header) {
	debug("\tHeader:\n");
	debug("\t\tmagic = %s\n", header->magic);
	debug("\t\tmapping_offset = %u\n", header->mapping_offset);
	debug("\t\tmapping_count = %u\n", header->mapping_count);
	debug("\t\timages_offset = %u\n", header->images_offset);
	debug("\t\timages_count = %u\n", header->images_count);
	debug("\t\tbase_address = 0x%qX\n", header->base_address);
	debug("\t\tcodesign_offset = 0x%qX\n", header->codesign_offset);
	debug("\t\tcodesign_size = %llu\n", header->codesign_size);
	debug("\n");
}

/*
 * Dyldcache Images Functions
 */
void dyldcache_images_debug(dyldcache_t* cache) {
	if(cache) {
		//debug("\tImages:\n");
		//debug("\t\taddress = 0x%qx\n", cache->images[0]->info->address);
		//debug("\n");
	}
}

void dyldcache_images_free(dyldcache_t* cache) {
	if(cache) {
		if(cache->images) {
			// TODO: Loop through each image and free it
			free(cache->images);
			cache->images = NULL;
		}
	}
}

/*
 * Dyldcache Maps Functions
 */
void dyldcache_maps_debug(dyldcache_t* cache) {
	if(cache) {
		//debug("\tMaps:\n");
		//debug("\t\taddress = 0x%qx\n", cache->maps[0]->info->address);
		//debug("\n");
	}
}

void dyldcache_maps_free(dyldcache_t* cache) {
	if(cache) {
		if(cache->maps) {
			// TODO: Loop through each map and free it
			free(cache->maps);
			cache->maps = NULL;
		}
	}
}
