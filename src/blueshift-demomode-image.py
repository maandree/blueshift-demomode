#!/usr/bin/env python3
copyright='''
blueshift-demomode — Blueshift-effect demonstration tools
Copyright Ⓒ 2014, 2015  Mattias Andrée (maandree@member.fsf.org)

This library is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this library.  If not, see <http://www.gnu.org/licenses/>.
'''


import os, sys

from argparser import *


PROGRAM_NAME = 'blueshift-demomode'
PROGRAM_VERSION = '1'
IMAGE_COMMAND = 'bin/blueshift-demomode-image'


parser = ArgParser('Blueshift-effect demonstration tools',
                   sys.argv[0] + ' (options | [--] images-file)',
                   None, None, True, ArgParser.standard_abbreviations())

parser.add_argumentless(['-h', '-?', '--help'], 0, 'Print this help information')
parser.add_argumentless(['-C', '--copying', '--copyright'], 0, 'Print copyright information')
parser.add_argumentless(['-W', '--warranty'], 0, 'Print non-warranty information')
parser.add_argumentless(['-v', '--version'], 0, 'Print program name and version')
    
parser.parse()
parser.support_alternatives()

if parser.opts['--help'] is not None:
    parser.help()
    sys.exit(0)
elif parser.opts['--copyright'] is not None:
    print(copyright[1 : -1])
    sys.exit(0)
elif parser.opts['--warranty'] is not None:
    print(copyright.split('\n\n')[2])
    sys.exit(0)
elif parser.opts['--version'] is not None:
    print('%s %s' % (PROGRAM_NAME, PROGRAM_VERSION))
    sys.exit(0)


if not len(parser.files) == 1:
    print('%s: you must select exactly one image file' % sys.argv[0], file = sys.stderr);


monitor = None
data = sys.stdin.buffer.read().decode('utf-8', 'strict').split('\n')

if monitor is not None:
    buf = []
    stage = 0
    for d in data:
        if stage == 0:
            if d.startswith('monitors: '):
                d = data[len('monitors: '):].split(' ')
                if monitor in d:
                    stage = 1
        elif stage == 1:
            if d.startswith('monitors: '):
                break
            buf.append(d)
    data = buf

data_max   = [d[len('max: '  ):] for d in data if d.startswith('max: '  )]
data_red   = [d[len('red: '  ):] for d in data if d.startswith('red: '  )]
data_green = [d[len('green: '):] for d in data if d.startswith('green: ')]
data_blue  = [d[len('blue: ' ):] for d in data if d.startswith('blue: ' )]

if any(map(lambda x : len(x) == 0, [data_max, data_red, data_green, data_blue])):
    print('%s: could not find any gamma ramp data' % sys.argv[0], file = sys.stderr);
    sys.exit(1)

data_max   = data_max[0]
data_red   = data_red[0]
data_green = data_green[0]
data_blue  = data_blue[0]


command = [IMAGE_COMMAND, data_max, data_red, data_green, data_blue]


os.close(0)
(read_end, write_end) = os.pipe()
pid = os.fork()

if pid == 0:
    os.close(read_end)
    if not write_end == 1:
        os.dup2(write_end, 1)
        os.close(write_end)
    fd = os.open(parser.files[0], os.O_RDONLY)
    if not fd == 0:
        os.dup2(fd, 0)
        os.close(fd)
    os.execvp('convert', ['convert', '-', 'pam:-'])
else:
    os.close(write_end)
    if not read_end == 0:
        os.dup2(read_end, 0)
        os.close(read_end)
    os.execvp(command[0], command)

