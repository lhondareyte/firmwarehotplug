#!/bin/bash
#
# Ancient traditional song
# Program: Copyright (C) 2002  Pedro Lopez-Cabanillas <plcl@bigfoot.com>
# based on a public domain musical arrangement by Aaron Fontaine.
# Download score from http://www.mutopiaproject.org
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
tun1="45 4,\
      48 2, 4a 4,\
      4c 3, 4d 8, 4c 4,\
      4a 2, 47 4,\
      43 3, 45 8, 47 4,\
      48 2, 45 4,\
      45 3, 44 8, 45 4,\
      47 2, 44 4,\
      40 2, 45 4,\
      48 2, 4a 4,\
      4c 3, 4d 8, 4c 4,\
      4a 2, 47 4,\
      43 3, 45 8, 47 4,\
      48 3, 47 8, 45 4,\
      44 3, 42 8, 44 4,\
      45 2, 0  4,\
      45 2, 0  4,\
      4f 2, 4f 4,\
      4f 3, 4d 8, 4c 4,\
      4a 2, 47 4,\
      43 3, 45 8, 47 4,\
      48 2, 45 4,\
      45 3, 44 8, 45 4,\
      47 2, 44 4,\
      40 2, 0  4,\
      4f 2, 4f 4,\
      4f 3, 4d 8, 4c 4,\
      4a 2, 47 4,\
      43 3, 45 8, 47 4,\
      48 3, 47 8, 45 4,\
      44 3, 42 8, 44 4,\
      45 2, 0  4,\
      45 1,"

tun2="0  4,\
      39 2, 3b 4,\
      3c 3, 0  8, 0 4,\
      37 2, 0  4,\
      3b 3, 0  8, 0 4,\
      39 2, 0  4,\
      41 3, 0  8, 0 4,\
      40 2, 0  4,\
      34 2, 0  4,\
      39 2, 3b 4,\
      3c 3, 0  8, 0 4,\
      37 2, 0  4,\
      3b 3, 0  8, 0 4,\
      39 3, 0  8, 0 4,\
      34 3, 0  8, 0 4,\
      39 2, 0  4,\
      39 2, 0  4,\
      3c 2, 0  4,\
      3c 3, 0  8, 0 4,\
      37 2, 0  4,\
      3b 3, 0  8, 0 4,\
      39 2, 0  4,\
      41 3, 0  8, 0 4,\
      40 2, 0  4,\
      34 2, 0  4,\
      3c 2, 0  4,\
      3c 3, 0  8, 0 4,\
      37 2, 0  4,\
      3b 3, 0  8, 0 4,\
      39 3, 0  8, 0 4,\
      34 3, 0  8, 0 4,\
      39 2, 0  4,\
      39 1,"

mididev="$1"
tempo=160 	# bpm
declare -a TIME

function computetimes() {
	for T in 1 2 3 4 8; do
		let "ms = 2400000 / ( $T * $tempo )"
		TIME[$T]=$ms'e-4s'
	done
}

function playnote() {
	if [ "$1" == "0" ]; then
		echo -ne "\x90\x$1\144" >/dev/null
		sleep ${TIME[$2]}
		echo -ne "\x80\x$1\000" >/dev/null
	else
		echo -ne "\x90\x$1\144" >&3
		sleep ${TIME[$2]}
		echo -ne "\x80\x$1\000" >&3
	fi
}

function playtune() {
	echo $1|while read -d, note length; do
			playnote $note $length
		done
}


if [ -c $mididev -a -w $mididev ]; then
	echo
	echo "This is \"Greensleaves\", an ancient traditional song"
	echo "based on a public domain arrangement by Aaron Fontaine."
	echo "Download the score from http://www.mutopiaproject.org"

	hash sleep 
	computetimes
	exec 3>$mididev
	echo -ne '\xb0\x07\x7f' >&3	# volume = 127
	echo -ne '\xc0\x18' >&3		# instrument = guitar
	playtune "$tun2"  &
	playtune "$tun1"
	exec 3<&-
	echo
else
	echo "`basename $0` : invalid MIDI device ( $mididev )"
fi
