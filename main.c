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

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/mman.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include "mobi.h"
#include "exth.h"
#include "pdb.h"

static void
usage(void)
{
    fprintf(stderr, "Usage: readmobi [-adDeEm] [-r id] file.mobi\n");
    fprintf(stderr, "\t-a\t\t\tprint all headers/records\n");
    fprintf(stderr, "\t-d\t\t\tprint PDB headers\n");
    fprintf(stderr, "\t-D\t\t\tprint PDB records\n");
    fprintf(stderr, "\t-e\t\t\tprint EXTH header\n");
    fprintf(stderr, "\t-E\t\t\tprint EXTH records\n");
    fprintf(stderr, "\t-m\t\t\tprint MOBI headers\n");
    fprintf(stderr, "\t-r record_id\t\tDump PDB record\n");
}

int
main(int argc, char **argv)
{
    int fd;
    void *ptr;
    unsigned char *mobi_data;
    char ch;

    off_t file_size;
    off_t file_pos = 0;
    off_t bytes_read;
    off_t record_offset;
    size_t record_size;

    int print_pdb_header = 0;
    int print_pdb_records = 0;
    int print_mobi_header = 0;
    int print_exth_header = 0;
    int print_exth_records = 0;
    int dump_record = -1;

    pdb_header_t *pdb_header;
    mobi_header_t *mobi_header;
    exth_header_t *exth_header;

    while ((ch = getopt(argc, argv, "adDeEmr:?")) != -1) {
        switch (ch) {
            case 'a':
                print_pdb_header = 1;
                print_pdb_records = 1;
                print_mobi_header = 1;
                print_exth_header = 1;
                print_exth_records = 1;
                break;
            case 'd':
                print_pdb_header = 1;
                break;
            case 'D':
                print_pdb_records = 1;
                break;
            case 'e':
                print_exth_header = 1;
                break;
            case 'E':
                print_exth_records = 1;
                break;
            case 'm':
                print_mobi_header = 1;
                break;
            case 'r':
                dump_record = atoi(optarg);
                break;
            case '?':
            default:
                usage();
                exit(0);
                break;
        }
    }

    if ((print_pdb_header || print_pdb_records ||
            print_mobi_header || print_exth_header || 
            print_exth_records) && (dump_record > -1)) {
        fprintf(stderr, "Can't mix -r and -adDeEm options\n");
        exit(0);
    }
    
    argc -= optind;
    argv += optind;

    if (argc < 1) {
        usage();
        exit(1);
    }

    fd = open(argv[0], O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    file_size = lseek(fd, 0, SEEK_END);
    ptr = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    mobi_data = (unsigned char*)ptr;

    pdb_header = pdb_header_alloc();

    bytes_read = pdb_header_read(pdb_header, mobi_data, file_size);
    if (bytes_read < 0) {
        fprintf(stderr, "pdb_header_read failed\n");
        exit(0);
    }

    file_size -= bytes_read;
    file_pos += bytes_read;
    
    if (print_pdb_header)
        pdb_header_print(pdb_header);

    if (print_pdb_records)
        pdb_header_print_records(pdb_header);

    mobi_header = mobi_header_alloc();
    bytes_read = mobi_header_read(mobi_header, (mobi_data + file_pos), file_size);
    if (bytes_read < 0) {
        fprintf(stderr, "mobi_header_read failed\n");
        exit(0);
    }

    file_size -= bytes_read;
    file_pos += bytes_read;

    if (print_mobi_header)
        mobi_header_print(mobi_header);

    exth_header = exth_header_alloc();
    bytes_read = exth_header_read(exth_header, (mobi_data + file_pos), file_size);
    if (bytes_read < 0) {
        fprintf(stderr, "exth_header_read failed\n");
        exit(0);
    }

    file_size -= bytes_read;
    file_pos += bytes_read;

    if (print_exth_header)
        exth_header_print(exth_header);

    if (print_exth_records)
        exth_header_print_records(exth_header);

    if (dump_record > -1) {
        record_offset = pdb_header_get_record_offset(pdb_header, dump_record);
        record_size = pdb_header_get_record_size(pdb_header, dump_record);
        if ((record_offset > 1) && (record_size > 0)) 
            write(fileno(stdout), mobi_data + record_offset, record_size);
        else
            fprintf(stderr, "PDB record #%d not found(%ld, %ld)\n", dump_record, record_offset, record_size);
    }

    munmap(ptr, file_size);

    return 0;
}
