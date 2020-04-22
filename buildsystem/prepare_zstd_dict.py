#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from sys import argv
from os.path import basename, getsize
from glob import glob
from datetime import datetime


dict_ext = '.zdict'
cols_octets_per_line = 12


def get_last_dict(dir_name, base_name):
    # file name has format: <base_name>-ddmmyy.zdict
    # where ddmmyy - dictionary creation date (day, month and year)
    def dicts_sort(val):
        date_str = val.split(dict_ext)[0].split('{}-'.format(base_name))[1]
        return datetime.strptime(date_str, '%d%m%y')

    dicts_list = glob('{}/{}-*{}'.format(dir_name, base_name, dict_ext))
    if len(dicts_list) > 1:
        # last dictionary will be first in the list
        dicts_list.sort(key=dicts_sort, reverse=True)

    return '' if not dicts_list else dicts_list[0]


def bytes_from_file(file_name, chunk_size=8192):
    with open(file_name, "rb") as f:
        while True:
            chunk = f.read(chunk_size)
            if chunk:
                for b in chunk:
                    yield b
            else:
                break


def generate_source_files_from_dict(dir_name, dict_name, source_name):
    resource_h_file = '{}/internal_{}.h'.format(dir_name, source_name)
    resource_cpp_file = '{}/internal_{}.cpp'.format(dir_name, source_name)
    source_name = source_name.encode('utf-8')

    with open(resource_h_file, 'wb') as result_h_file:
        result_h_file.write(b'#pragma once\n\nnamespace zstd_internal\n{\n')
        result_h_file.write(b'\tconstexpr const char* get_%s_name() noexcept { return "%s"; }\n'
                            % (source_name, basename(dict_name).encode('utf-8')))
        result_h_file.write(b'\tusing %s_t = std::array<unsigned char, %d>;\n'
                            % (source_name, getsize(dict_name)))
        result_h_file.write(b'\tconst %s_t& get_%s();\n}\n' % (source_name, source_name))

    col = 0
    with open(resource_cpp_file, 'wb') as result_cpp_file:
        result_cpp_file.write(b'#include "stdafx.h"\n#include "%s"\n\n' % basename(resource_h_file).encode('utf-8'))
        result_cpp_file.write(b'const zstd_internal::%s_t& zstd_internal::get_%s()\n{\n' % (source_name, source_name))
        result_cpp_file.write(b'\tstatic constexpr zstd_internal::%s_t %s = {\n\t' % (source_name, source_name))

        for b in bytes_from_file(dict_name):
            if col > 0 and col % 12 == 0:
                result_cpp_file.write(b',\n\t')
                col = 0

            if col > 0:
                result_cpp_file.write(b', ')
            else:
                result_cpp_file.write(b'\t')

            result_cpp_file.write(b'0x%02X' % b)

            col += 1

        result_cpp_file.write(b'\n\t};\n\treturn %s;\n}\n' % source_name)


if __name__ == '__main__':

    if len(argv) < 4:
        print('Wrong command line arguments!')
        print('Usage:\n\t{} DICTIONARY_DIR DICTIONARY_BASE_FILE_NAME GENERATE_SOURCE_NAME'.format(basename(argv[0])))
        exit(1)

    dicts_dir = argv[1]
    dict_base_name = argv[2]
    internal_source_name = argv[3]

    last_dict = get_last_dict(dicts_dir, dict_base_name)
    if not last_dict:
        print('Dictionary not found in directory {}!\n'.format(dicts_dir))
        exit(1)

    generate_source_files_from_dict(dicts_dir, last_dict, internal_source_name)

    exit(0)
