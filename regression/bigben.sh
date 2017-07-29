#!/bin/bash
#
# London Tower Big Ben tune as a simple bash script
# Copyright (C) 2002  Pedro Lopez-Cabanillas <plcl@bigfoot.com>
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
tune="C 4, E 4, D 4, g 2, C 4, D 4, E 4, C 2, \
      E 4, C 4, D 4, g 2, g 4, D 4, E 4, C 2,"

mididev="$1"
tempo=80 				# quarters per minute

function playnote() {
	let "ms = 240000 / ($2 * $tempo)"
	echo -ne '\x90' >&3
	echo -ne "$1\144" >&3
	sleep $ms'e-3s'
	echo -ne '\x80' >&3
	echo -ne "$1\000" >&3
}

function playtune() {
	echo $1 | tr "cdefgabCDEFGAB" "<>@ACEGHJLMOQS" | \
		while read -d, note length; do
			playnote $note $length
		done
}

if [ -c $mididev -a -w $mididev ]; then
	exec 3>$mididev
	echo -ne '\xb0\x07\x7f' >&3	# volume = 127
	echo -ne '\xc0\x0e' >&3		# instrument = bells
	playtune "$tune"
	exec 3<&-
else
	echo "`basename $0` : invalid MIDI device ( $mididev )"
fi
