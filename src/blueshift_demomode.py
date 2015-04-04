# -*- python -*-
'''
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


import time

import monitor
import aux

real = time.clock_gettime(time.CLOCK_REALTIME)
mraw = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)

print('start time: real: %f' % real)
print('start time: measured: %f' % mraw)
print()

def set_gamma_(*crtcs, screen = 0, **kwargs):
    old_set_gamma(*crtcs, screen = screen, **kwargs)
    (R_curve, G_curve, B_curve) = aux.translate_to_integers()
    mraw = time.clock_gettime(time.CLOCK_MONOTONIC_RAW)
    print('time: %f'  % mraw)
    print('monitors: %s'  % ' '.join('%i.%i' % (screen, crtc) for crtc in crtcs))
    print('max: %i %i %i'  % (2**16 - 1, 2**16 - 1, 2**16 - 1))
    print('red: %s'   % ' '.join([str(stop) for stop in R_curve]))
    print('green: %s' % ' '.join([str(stop) for stop in G_curve]))
    print('blue: %s'  % ' '.join([str(stop) for stop in B_curve]))
    print()
old_set_gamma = monitor.set_gamma
monitor.set_gamma = set_gamma_

