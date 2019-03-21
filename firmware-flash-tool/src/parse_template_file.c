#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h>

#include <json-c/json.h>

#include "esperanto_flash_tool.h"
#include "parse_template_file.h"

#define IMAGE_KEY "image"
#define IMAGE_SIZE_KEY "image_size"
#define IMAGE_PARTITIONS_KEY "partitions"
#define PARTITION_KEY "partition"
#define PARTITION_SIZE_KEY "partition_size"
#define PARTITION_PRIORITY_KEY "priority"
#define PARTITION_ATTEMPTED_COUNT_KEY "attempted_boot_count"
#define PARTITION_COMPLETED_COUNT_KEY "completed_boot_count"
#define PARTITION_REGIONS_KEY "regions"
#define REGION_ID_KEY "id"
#define REGION_SIZE_KEY "size"
#define REGION_FILE_KEY "file"

typedef struct REGION_INFO {
    ESPERANTO_FLASH_REGION_ID_t id;
    const char * name;
} REGION_INFO_t;

static REGION_INFO_t regions_info[] = {
    { ESPERANTO_FLASH_REGION_ID_PRIORITY_DESIGNATOR, "PRIORITY_DESIGNATOR" },
    { ESPERANTO_FLASH_REGION_ID_BOOT_COUNTERS, "BOOT_COUNTERS" },
    { ESPERANTO_FLASH_REGION_ID_SP_CERTIFICATES, "SP_CERTIFICATES" },
    { ESPERANTO_FLASH_REGION_ID_SW_CERTIFICATES, "SW_CERTIFICATES" },
    { ESPERANTO_FLASH_REGION_ID_COMM_CERTIFICATES, "COMM_CERTIFICATES" },
    { ESPERANTO_FLASH_REGION_ID_CONFIGURATION_DATA, "CONFIGURATION_DATA" },
    { ESPERANTO_FLASH_REGION_ID_SP_BL1, "SP_BL1" },
    { ESPERANTO_FLASH_REGION_ID_SP_BL2, "SP_BL2" },
    { ESPERANTO_FLASH_REGION_ID_DRAM_TRAINING, "DRAM_TRAINING" },
    { ESPERANTO_FLASH_REGION_ID_MASTER_MINION_EXEC, "MASTER_MINION_EXEC" },
    { ESPERANTO_FLASH_REGION_ID_MASTER_MINION_USER, "MASTER_MINION_USER" },
    { ESPERANTO_FLASH_REGION_ID_WORKER_MINION_EXEC, "WORKER_MINION_EXEC" },
    { ESPERANTO_FLASH_REGION_ID_MAXION_BL1, "MAXION_BL1" }
};
static const uint32_t regions_count = sizeof(regions_info) / sizeof(REGION_INFO_t);

static ESPERANTO_FLASH_REGION_ID_t region_name_to_id(const char * name) {
    uint32_t n;
    for (n = 0; n < regions_count; n++) {
        if (0 == strcasecmp(regions_info[n].name, name)) {
            return regions_info[n].id;
        }
    }
    return ESPERANTO_FLASH_REGION_ID_INVALID;
}

static int process_integer(json_object * value, uint32_t * num) {
    unsigned long int n;
    const char * str;
    char * endptr;
    if (json_type_string == json_object_get_type(value)) {
        str = json_object_get_string(value);
        if (NULL == str) {
            fprintf(stderr, "ERROR in process_integer: REGION ID is a NULL string!\n");
            return -1;
        }
        n = strtoul(str, &endptr, 0);
        if (0 != *endptr) {
            fprintf(stderr, "ERROR in process_integer: Invalid numeric value '%s'!\n", str);
            return -1;
        }
        *num = (uint32_t)n;
        return 0;
    } else if (json_type_int == json_object_get_type(value)) {
        *num = (uint32_t)json_object_get_int(value);
        return 0;
    } else {
        fprintf(stderr, "ERROR in process_integer: Invalid value type!\n");
        return -1;
    }
}

static int process_region_id(json_object * value, ESPERANTO_FLASH_REGION_ID_t * pid) {
    ESPERANTO_FLASH_REGION_ID_t id;
    const char * str;

    if (json_type_string == json_object_get_type(value)) {
        str = json_object_get_string(value);
        if (NULL == str) {
            fprintf(stderr, "ERROR in process_region_id: REGION ID is a NULL string!\n");
            return -1;
        }
        id = region_name_to_id(str);
        if (ESPERANTO_FLASH_REGION_ID_INVALID != id) {
            *pid = id;
            return 0;
        }

    } else {
        return process_integer(value, pid);
    }

}

static void free_region(REGION_INFO_t * pregion) {
    if (NULL != pregion->name) {
        free(pregion->name);
        pregion->name = NULL;
    }
}
static int process_region(json_object * jobj, REGION_INFO_t * pregion) {
    json_object * value;
    const char * str;

    if (TRUE != json_object_object_get_ex(jobj, REGION_ID_KEY, &value)) {
        fprintf(stderr, "ERROR in process_region: Missing key " REGION_ID_KEY "!\n");
        return -1;
    }
    if (0 != process_region_id(value, &(pregion->id))) {
        fprintf(stderr, "ERROR in process_region: process_region_id(0 failed!\n");
        return -1;
    }

    if (TRUE == json_object_object_get_ex(jobj, REGION_SIZE_KEY, &value)) {
        if (0 != process_integer(value, &(pregion->size))) {
            fprintf(stderr, "ERROR in process_region: process_integer() failed to parse region size!\n");
            return -1;
        }
    }

    if (TRUE == json_object_object_get_ex(jobj, REGION_FILE_KEY, &value)) {
        if (json_type_string == json_object_get_type(value)) {
            str = json_object_get_string(value);
            if (NULL == str) {
                fprintf(stderr, "ERROR in process_region: FILE PATH is a NULL string!\n");
                return -1;
            }
            pregion->name = (char*)malloc(1 + strlen(str));
            if (NULL == pregion->name) {
                fprintf(stderr, "ERROR in process_region: malloc(0 failed!\n");
                return -1;
            }
            strcpy(pregion->name, str);
        } else {
            fprintf(stderr, "ERROR in process_region: '" REGION_FILE_KEY "' is not a string type!\n");
            return -1;
        }
    } else {
        pregion->name = NULL;
    }

    return 0;
}

static int process_regions_list(json_object * jobj, REGION_INFO_t ** ppregions, uint32_t * pregions_count) {
    int rv;
    json_object * jelement;
    uint32_t count, index, n, valid_count = 0;
    REGION_INFO_t * regions = NULL;

    if (json_type_array != json_object_get_type(jobj)) {
        fprintf(stderr, "ERROR in process_regions_list: json object is not an ARRAY type!\n");
        rv = -1;
        goto DONE;
    }

    count = (uint32_t)json_object_array_length(jobj);
    regions = (REGION_INFO_t*)malloc(count * sizeof(REGION_INFO_t));
    if (NULL == regions) {
        fprintf(stderr, "ERROR in process_regions_list: malloc() failed!\n");
        rv = -1;
        goto DONE;
    }
    memset(regions, 0, count * sizeof(REGION_INFO_t));

    for (index = 0; index < count; index++) {
        jelement = json_object_array_get_idx(jobj, index);
        if (NULL == jelement) {
            fprintf(stderr, "ERROR in process_regions_list: json_object_array_get_idx(%u) returned NULL!\n", index);
            rv = -1;
            goto DONE;
        }

        if (0 != process_region(jelement, &regions[index])) {
            fprintf(stderr, "ERROR in process_regions_list: process_region(%u) failed!\n", index);
            rv = -1;
            goto DONE;
        }
        valid_count++;

        for (n = 0; n < index; n++) {
            if (regions[n].id == regions[index].id) {
                fprintf(stderr, "ERROR in process_regions_list: region %u has the same id (0x%x) as region %u!\n", index, regions[index].id, n);
                rv = -1;
                goto DONE;
            }
        }
    }

    *pregions_count = count;
    *ppregions = regions;
    regions = NULL;
    rv = 0;

DONE:
    if (NULL != regions) {
        for (n = 0; n < valid_count; n++) {
            free_region(&regions[n]);
        }
        free(regions);
    }
    return rv;
}

static void free_partition(PARTITION_INFO_t * ppartition) {
    uint32_t n;
    for (n = 0; n < ppartition->regions_count; n++) {
        free_region(&ppartition->regions[n]);
    }
    free(ppartition->regions[n]);
}
static int process_partition(json_object * jobj, PARTITION_INFO_t * ppartition) {
    int rv;
    json_object * value;

    if (TRUE == json_object_object_get_ex(jobj, PARTITION_SIZE_KEY, &value)) {
        if (0 != process_integer(value, &(ppartition->partition_size))) {
            fprintf(stderr, "ERROR in process_partition: process_integer() failed to parse partition size!\n");
            rv = -1;
            goto DONE;
        }
    } else {
        ppartition->partition_size = 0;
    }

    if (TRUE == json_object_object_get_ex(jobj, PARTITION_PRIORITY_KEY, &value)) {
        if (0 != process_integer(value, &(ppartition->priority))) {
            fprintf(stderr, "ERROR in process_partition: process_integer() failed to parse partition priority!\n");
            rv = -1;
            goto DONE;
        }
    } else {
        ppartition->priority = 0;
    }

    if (TRUE == json_object_object_get_ex(jobj, PARTITION_ATTEMPTED_COUNT_KEY, &value)) {
        if (0 != process_integer(value, &(ppartition->attempted_boot_counter))) {
            fprintf(stderr, "ERROR in process_partition: process_integer() failed to parse partition attempted boot count!\n");
            rv = -1;
            goto DONE;
        }
    } else {
        ppartition->attempted_boot_counter = 0;
    }

    if (TRUE == json_object_object_get_ex(jobj, PARTITION_COMPLETED_COUNT_KEY, &value)) {
        if (0 != process_integer(value, &(ppartition->completed_boot_counter))) {
            fprintf(stderr, "ERROR in process_partition: process_integer() failed to parse partition completed boot count!\n");
            rv = -1;
            goto DONE;
        }
    } else {
        ppartition->completed_boot_counter = 0;
    }

    if (TRUE != json_object_object_get_ex(jobj, PARTITION_REGIONS_KEY, &value)) {
        fprintf(stderr, "ERROR in process_partition: missing key '" PARTITION_REGIONS_KEY "'!\n");
        rv = -1;
        goto DONE;
    }

    if (0 != process_regions_list(value, &(ppartition->regions), &(ppartition->regions_count))) {
        fprintf(stderr, "ERROR in process_partition: process_regions_list() failed!\n");
        rv = -1;
        goto DONE;
    }

    rv = 0;

DONE:
    return rv;
}

static void free_image(IMAGE_INFO_t * pimage) {
    free_partition(&(pimage->partitions[0]));
    free_partition(&(pimage->partitions[1]));
}

static int process_image(json_object * jobj, IMAGE_INFO_t * pimage) {
    int rv;
    json_object * value;
    json_object * partition;
    uint32_t count;
    uint32_t index = 0;

    if (TRUE == json_object_object_get_ex(jobj, PARTITION_SIZE_KEY, &value)) {
        if (0 != process_integer(value, &(pimage->image_size))) {
            fprintf(stderr, "ERROR in process_image: process_integer() failed to parse image size!\n");
            rv = -1;
            goto DONE;
        }
    } else {
        pimage->image_size = 0;
    }

    if (TRUE != json_object_object_get_ex(jobj, IMAGE_PARTITIONS_KEY, &value)) {
        fprintf(stderr, "ERROR in process_image: missing key '" IMAGE_PARTITIONS_KEY "'!\n");
        rv = -1;
        goto DONE;
    }

    if (json_type_array != json_object_get_type(jobj)) {
        fprintf(stderr, "ERROR in process_image: json object '" IMAGE_PARTITIONS_KEY "' is not an ARRAY type!\n");
        rv = -1;
        goto DONE;
    }

    count = (uint32_t)json_object_array_length(jobj);
    if (2 != count) {
        fprintf(stderr, "ERROR in process_image: partitions list is not of size 2!\n");
        rv = -1;
        goto DONE;
    }

    for (; index < 2; index++) {
        partition = json_object_array_get_idx(jobj, index);
        if (NULL == partition) {
            fprintf(stderr, "ERROR in process_image: json_object_array_get_idx(%u) returned NULL!\n", index);
            rv = -1;
            goto DONE;
        }
        if (0 != process_partition(partition, &(pimage->partitions[index]))) {
            fprintf(stderr, "ERROR in process_image: process_partition() failed while processing partition %u!\n", index);
            rv = -1;
            goto DONE;
        }
    }

    rv = 0;

DONE:
    if (0 != rv && index > 0) {
        free_partition(&(pimage->partitions[0]));
    }
    return rv;
}

static int load_file(const char * file_path, char ** buffer, size_t * buffer_size) {
    int rval = 0;
    long offset;
    size_t size;
    FILE * f;
    char * ptr;

    if (NULL == buffer || NULL == buffer_size) {
        fprintf(stderr, "ERROR in load_file: INVALID ARGUMENTS!\n");
        rval = -1;
        goto FAILED1;
    }
    
    f = fopen(file_path, "r");
    if (NULL == f) {
        fprintf(stderr, "ERROR in load_file: fopen(\"%s\", \"r\") failed!\n", file_path);
        rval = -1;
        goto FAILED1;
    }

    if (-1 == fseek(f, 0L, SEEK_END)) {
        fprintf(stderr, "ERROR in load_file: fseek(SEEK_END) failed!\n");
        fclose(f);
        rval = -1;
        goto FAILED2;
    }

    offset = ftell(f);
    if (-1 == offset) {
        fprintf(stderr, "ERROR in load_file: ftell() failed!\n");
        rval = -1;
        goto FAILED2;
    }
    size = (size_t)offset;

    if (-1 == fseek(f, 0L, SEEK_SET)) {
        fprintf(stderr, "ERROR in load_file: fseek(SEEK_SET) failed!\n");
        fclose(f);
        rval = -1;
        goto FAILED2;
    }

    ptr = (char*)malloc(size);
    if (NULL == ptr) {
        fprintf(stderr, "ERROR in load_file: malloc(%lu) failed!\n", size);
        rval = -1;
        goto FAILED2;
    }

    if (size != fread(ptr, 1, size, f)) {
        fprintf(stderr, "ERROR in load_file: fread(%lu) failed!\n", size);
        rval = -1;
        goto FAILED3;
    }

    *buffer_size = size;
    *buffer = ptr;
    fclose(f);
    return 0;

FAILED3:
    free(ptr);

FAILED2:
    fclose(f);

FAILED1:
    return rval;
}

void free_template_info(TEMPLATE_INFO_t * template) {

}

int parse_template_file(const char * filename, TEMPLATE_INFO_t * template) {
    int rv;
    char * buffer = NULL;
    size_t buffer_size;

    struct json_object * jobj, *value;
    enum json_tokener_error jerror;
    struct json_tokener * tokener = NULL;
    const char * error_desc;

    if (0 != load_file(filename, &buffer, &buffer_size)) {
        fprintf(stderr, "ERROR in parse_template_file: failed to load file '%s'!\n", filename);
        rv = -1;
        goto DONE;
    }

    tokener = json_tokener_new();
    if (NULL == tokener) {
        fprintf(stderr, "ERROR in parse_template_file: json_tokener_new() failed!\n");
        rv = -1;
        goto DONE;
    }

    jobj = json_tokener_parse_ex(tokener, buffer, (int)buffer_size);
    if (NULL == jobj) {
        fprintf(stderr, "ERROR in parse_template_file: json_tokener_parse_ex() failed!\n");
        jerror = json_tokener_get_error(tokener);
        error_desc = json_tokener_error_desc(jerror);
        if (NULL != error_desc) {
            fprintf(stderr, "json_tokener_error_desc: %s\n", error_desc);
        }
        rv = -1;
        goto DONE;
    }

    if (TRUE == json_object_object_get_ex(jobj, IMAGE_KEY, &value)) {
        template->image = (IMAGE_INFO_t*)malloc(sizeof(IMAGE_INFO_t));
        if (NULL == template->image) {
            fprintf(stderr, "ERROR in parse_template_file: failed to allocate IMAGE_INFO_t memory!\n");
            rv = -1;
            goto DONE;
        }
        if (0 != process_image(value, template->image)) {
            free(template->image);
            template->image = NULL;
            fprintf(stderr, "ERROR in parse_template_file: process_image() failed!\n");
            rv = -1;
            goto DONE;
        }
        template->image_type = true;
    } else if (TRUE == json_object_object_get_ex(jobj, PARTITION_KEY, &value)) {
        template->partition = (PARTITION_INFO_t*)malloc(sizeof(PARTITION_INFO_t));
        if (NULL == template->partition) {
            fprintf(stderr, "ERROR in parse_template_file: failed to allocate PARTITION_INFO_t memory!\n");
            rv = -1;
            goto DONE;
        }
        if (0 != process_partition(value, template->partition)) {
            free(template->partition);
            template->image = NULL;
            fprintf(stderr, "ERROR in parse_template_file: process_partition() failed!\n");
            rv = -1;
            goto DONE;
        }
        template->image_type = false;
    } else {
        fprintf(stderr, "ERROR in parse_template_file: could not find '" IMAGE_KEY "' or '" PARTITION_KEY "' tags!\n");
        rv = -1;
        goto DONE;
    }

    rv = 0;

DONE:
    if (NULL != tokener) {
        json_tokener_free(tokener);
    }

    return rv;
}
