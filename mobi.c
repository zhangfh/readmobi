/*-
 * Copyright (c) 2012 Oleksandr Tymoshenko <gonzo@bluezbox.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mobi.h"
#include "bytestream.h"
#include "utils.h"

#define MIN_MOBI_HEADER_SIZE (16 + 0xf8)

/*
 * Just to be on the safe side
 */
#ifdef MOBI_HEADER_READ_2
#undef  MOBI_HEADER_READ_2
#endif

#ifdef MOBI_HEADER_READ_4
#undef  MOBI_HEADER_READ_4
#endif

#define MOBI_HEADER_READ_2(v, ptr) do {     \
    (v) = bs_read_2((ptr)); (ptr) += 2;     \
} while (0);

#define MOBI_HEADER_READ_4(v, ptr) do {     \
    (v) = bs_read_4((ptr)); (ptr) += 4;     \
} while (0);

mobi_header_t*
mobi_header_alloc()
{
    mobi_header_t *h = malloc(sizeof(mobi_header_t));
    memset(h, 0, sizeof(*h));
    return h;
}

void
mobi_header_free(mobi_header_t* header)
{
    free(header);
}

void
mobi_header_print(mobi_header_t* h)
{
    int i;

    printf("PalmDOC header\n");
    printf("  Compression: %d", h->mobi_compression);
    switch (h->mobi_compression) {
        case 1:
            printf(" (no compression)");
            break;
        case 2:
            printf(" (PalmDOC compression)");
            break;
        case 17480:
            printf(" (HUFF/CDC compression)");
            break;
        default:
            printf(" (Unknown compression)");
            break;
    }
    printf("\n");
    printf("  Uncompressed text length: %d\n", h->mobi_text_length);
    printf("  Record count: %d\n", h->mobi_record_count);
    printf("  Record size: %d\n", h->mobi_record_size);
    printf("  Encryptin type: %d\n", h->mobi_encryption_type);
    printf("MOBI header\n");
    printf("  MOBI ID: %08x (%s)\n", h->mobi_indetifier, id2string(h->mobi_indetifier));
    printf("  MOBI header length: %d\n", h->mobi_header_length);
    /* TODO: print proper type here */
    printf("  Type %08x\n", h->mobi_type);
    printf("  Text encoding: %d\n", h->mobi_text_encoding);
    printf("  UniqID: %08x\n", h->mobi_uid);
    printf("  File version: %d\n", h->mobi_file_version);
    if (h->mobi_ortographic_index != MOBI_NO_INDEX)
        printf("  Ortographic index: %d\n", h->mobi_ortographic_index);
    if (h->mobi_inflection_index != MOBI_NO_INDEX)
        printf("  Inflection index: %d\n", h->mobi_inflection_index);
    if (h->mobi_index_names != MOBI_NO_INDEX)
        printf(" Index names:  %d\n", h->mobi_index_names);
    if (h->mobi_index_keys != MOBI_NO_INDEX)
        printf("  Index keys: %d\n", h->mobi_index_keys);
    for (i = 0; i < MOBI_EXTRA_INDEXES; i++) {
        if (h->mobi_extra_index[i] != MOBI_NO_INDEX)
            printf("  Extra Index #%d: %d\n", i, h->mobi_extra_index[i]);
    }
    printf("  First non-book index: %d\n", h->mobi_first_nonbook_index);
    printf("  Full name offset: %d\n", h->mobi_full_name_offset);
    printf("  Full name length: %d\n", h->mobi_full_name_length);
    printf("  Locale: %d\n", h->mobi_locale);
    printf("  Dict input length: %d\n", h->mobi_dict_input_lang);
    printf("  Dict output length: %d\n", h->mobi_dict_output_lang);
    printf("  Min version: %d\n", h->mobi_min_version);
    printf("  First image index: %d\n", h->mobi_first_image_rec);
    printf("  Huffman record offset: %d\n", h->mobi_huffman_record_offset);
    printf("  Huffman record count: %d\n", h->mobi_huffman_record_count);
    printf("  Huffman table offset: %d\n", h->mobi_huffman_table_offset);
    printf("  Huffman table count: %d\n", h->mobi_huffman_table_count);
    printf("  EXTH flags: %s (0x%08x)\n", 
            ((h->mobi_exth_flags & MOBI_EXTH_PRESENT) ? "EXTH present" : "No EXTH"),
            h->mobi_exth_flags);
    printf("  DRM offset: %d\n", h->mobi_drm_offset);
    printf("  DRM count: %d\n", h->mobi_drm_count);
    printf("  DRM size: %d\n", h->mobi_drm_size);
    printf("  DRM flags: %08x\n", h->mobi_drm_flags);
    printf("  First content record #: %d\n", h->mobi_first_content_rec);
    printf("  Last content record #: %d\n", h->mobi_last_content_rec);
    printf("  FCIS record #: %d\n", h->mobi_fcis_rec);
    printf("  FLIS record #: %d\n", h->mobi_flis_rec);
    printf("  Extra record data flags:");
    if (h->mobi_extra_record_data_flags & EXTH_EXTRA_MULTIBYTE)
        printf(" multibyte");
    if (h->mobi_extra_record_data_flags & EXTH_EXTRA_TBS_INDEX)
        printf(" tbs_index");
    if (h->mobi_extra_record_data_flags & EXTH_EXTRA_UNCROSSABLE)
        printf(" uncrossable_breaks");
    printf(" (0x%08x)\n", h->mobi_extra_record_data_flags);
    printf("  INDX record offset: %d\n", h->mobi_indx_record_offset);
}

off_t
mobi_header_read(mobi_header_t* h, unsigned char *ptr, off_t size)
{
    int i;

    if (size < MIN_MOBI_HEADER_SIZE)
        return (-1);

    MOBI_HEADER_READ_2(h->mobi_compression, ptr);

    /* Skip 2 unused bytes */
    ptr += 2;

    MOBI_HEADER_READ_4(h->mobi_text_length, ptr);
    MOBI_HEADER_READ_2(h->mobi_record_count, ptr);
    MOBI_HEADER_READ_2(h->mobi_record_size, ptr);
    MOBI_HEADER_READ_2(h->mobi_encryption_type, ptr);

    /* zeroes */
    ptr += 2;

    MOBI_HEADER_READ_4(h->mobi_indetifier, ptr);
    MOBI_HEADER_READ_4(h->mobi_header_length, ptr);
    MOBI_HEADER_READ_4(h->mobi_type, ptr);
    MOBI_HEADER_READ_4(h->mobi_text_encoding, ptr);
    MOBI_HEADER_READ_4(h->mobi_uid, ptr);
    MOBI_HEADER_READ_4(h->mobi_file_version, ptr);
    MOBI_HEADER_READ_4(h->mobi_ortographic_index, ptr);
    MOBI_HEADER_READ_4(h->mobi_inflection_index, ptr);
    MOBI_HEADER_READ_4(h->mobi_index_names, ptr);
    MOBI_HEADER_READ_4(h->mobi_index_keys, ptr);

    for (i = 0; i < MOBI_EXTRA_INDEXES; i++) {
        MOBI_HEADER_READ_4(h->mobi_extra_index[i], ptr);
    }

    MOBI_HEADER_READ_4(h->mobi_first_nonbook_index, ptr);
    MOBI_HEADER_READ_4(h->mobi_full_name_offset, ptr);
    MOBI_HEADER_READ_4(h->mobi_full_name_length, ptr);
    MOBI_HEADER_READ_4(h->mobi_locale, ptr);
    MOBI_HEADER_READ_4(h->mobi_dict_input_lang, ptr);
    MOBI_HEADER_READ_4(h->mobi_dict_output_lang, ptr);
    MOBI_HEADER_READ_4(h->mobi_min_version, ptr);
    MOBI_HEADER_READ_4(h->mobi_first_image_rec, ptr);
    MOBI_HEADER_READ_4(h->mobi_huffman_record_offset, ptr);
    MOBI_HEADER_READ_4(h->mobi_huffman_record_count, ptr);
    MOBI_HEADER_READ_4(h->mobi_huffman_table_offset, ptr);
    MOBI_HEADER_READ_4(h->mobi_huffman_table_count, ptr);
    MOBI_HEADER_READ_4(h->mobi_exth_flags, ptr);

    /* Skip 32 empty bytes of unknown purpose */
    ptr += 32;

    MOBI_HEADER_READ_4(h->mobi_drm_offset, ptr);
    MOBI_HEADER_READ_4(h->mobi_drm_count, ptr);
    MOBI_HEADER_READ_4(h->mobi_drm_size, ptr);
    MOBI_HEADER_READ_4(h->mobi_drm_flags, ptr);

    /* Skip junk */
    ptr += 12;

    MOBI_HEADER_READ_2(h->mobi_first_content_rec, ptr);
    MOBI_HEADER_READ_2(h->mobi_last_content_rec, ptr);

    /* Skip junk */
    ptr += 4;

    MOBI_HEADER_READ_4(h->mobi_fcis_rec, ptr);
    ptr += 4; /* unknown, just 1 */
    MOBI_HEADER_READ_4(h->mobi_flis_rec, ptr);
    ptr += 4; /* unknown, just 1 */

    ptr += 24; /* misc stuff */

    MOBI_HEADER_READ_4(h->mobi_extra_record_data_flags, ptr);
    if (h->mobi_header_length == 0xe8) {
        MOBI_HEADER_READ_4(h->mobi_indx_record_offset, ptr);
    } else
        h->mobi_indx_record_offset = 0xffffffff;

    return h->mobi_header_length + 16;
}
