#!/bin/bash
#
# Functions for testing and selecting available midi devices.
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

function create_log() {
	LOG=testlog.$(date +%Y%m%d%H%M%S)
	echo "Regression test for MIDI devices" >$LOG
	echo " " >>$LOG
	echo "executed by $(whoami)" >>$LOG
	echo "on system $(uname -a)" >>$LOG
	echo "at $(date -R)" >>$LOG
	echo " " >>$LOG
	echo "Log created: $LOG"
	LOGCREATED=1
}

function module_info() {
	MINFO=$(whereis modinfo|awk '{print $2}')
	if [ -x $MINFO ]; then
		$MINFO -a $1|tee -a $LOG
		$MINFO -d $1|tee -a $LOG
		$MINFO -n $1|tee -a $LOG
	fi
}

function makelist() {
	unset LISTDEV
	for D in /dev/midi*; do
		./midiraw $1 $D && LISTDEV="$LISTDEV $D";
	done
}

function getindev() {
	makelist -ni
	echo "Please, select an INPUT device: "
	select INDEV in $LISTDEV; do
		./midiraw -ni $INDEV 2>/dev/null
		if [ $? == 0 ]; then
			break;
		else
			echo "invalid selection, please retry"
		fi
	done
}

function getoutdev() {
	makelist -no
	echo "Please, select an OUTPUT device: "
	select OUTDEV in $LISTDEV; do
		./midiraw -no $OUTDEV 2>/dev/null
		if [ $? == 0 ]; then
			break;
		else
			echo "invalid selection, please retry"
		fi
	done
}

RES_COL=60
MOVE_TO_COL="echo -en \\033[${RES_COL}G"
SETCOLOR_SUCCESS="echo -en \\033[1;32m"
SETCOLOR_FAILURE="echo -en \\033[1;31m"
SETCOLOR_WARNING="echo -en \\033[1;33m"
SETCOLOR_NORMAL="echo -en \\033[0;39m"

function echo_pass() {
	$MOVE_TO_COL
	echo -n "[ "
	$SETCOLOR_SUCCESS
	echo -n "PASS"
	$SETCOLOR_NORMAL
	echo " ]"
}

function echo_fail() {
	$MOVE_TO_COL
	echo -n "[ "
	$SETCOLOR_FAILURE
	echo -n "FAIL"
	$SETCOLOR_NORMAL
	echo " ]"
}

function showresult() {
	if [ $? == 0 ]; then
		echo -n "Test $1 : $2"; echo_pass
		echo "Test $1 PASS ($2)" >>$LOG
		return 0
	else
		echo -n "Test $1 : $2"; echo_fail
		echo "Test $1 FAIL ($2)" >>$LOG
		return 1
	fi
}
