#!/bin/bash
#
# Little test on MIDI out ports.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
echo
echo "You should hear a note A from your external MIDI synth"
echo "GM Instruments shuld play this note with a grand piano sound"
echo "Volume (max)"
echo -n $'\xb0\x07\x7f' > $1
echo "Program change 1 (GM Piano 2)"
echo -n $'\xc0\x01' > $1
echo "Note on \"A\" (440Hz)"
echo -n $'\x90\x45\x70' > $1
sleep 2s
echo "Note off"
echo -n $'\x80\x45\x70' > $1
echo
