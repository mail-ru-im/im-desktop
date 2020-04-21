#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from sys import argv, exit
from os import getcwd, listdir
from os.path import join, isfile, basename
from hashlib import md5
from re import match


def list_files(basedir=None, files_mask=None):
    if basedir is None:
        basedir = getcwd()

    for item in listdir(basedir):
        path = join(basedir, item)
        if isfile(path):
            if files_mask and not match(files_mask, path):
                continue
            yield path


def md5sum(f, block_size=None):
    if block_size is None:
        block_size = 4096

    hash = md5()
    with open(f, 'rb') as fh:
        block = fh.read(block_size)
        while block:
            hash.update(block)
            block = fh.read(block_size)

    return hash.hexdigest()


def md5sums(basedir=None, files_mask=None, block_size=None):
    for f in list_files(basedir=basedir, files_mask=files_mask):
        yield (md5sum(f, block_size=block_size), basename(f))


if __name__ == '__main__':

    basedir = None if len(argv) < 2 else argv[1]
    mask = None if len(argv) < 3 else argv[2]
    if len(argv) > 3:
        print('Wrong command line arguments!')
        print('Usage:\n\t{} BASE_DIR MASK_PATTERN'.format(basename(argv[0])))
        exit(1)

    for file_hash in md5sums(basedir, mask):
        print('\t'.join(file_hash))

    exit(0)
