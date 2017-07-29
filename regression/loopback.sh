#!/bin/bash
#
# Regression tests for MIDI
# (Loopback tests)
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
if [ -z $LOGCREATED ]; then
	. functions.sh
	create_log
fi

echo "*******************************************************************"|tee -a $LOG
echo "*                        loopback tests                           *"|tee -a $LOG
echo "*******************************************************************"|tee -a $LOG

echo "Please, connect input to output using a MIDI cable"
getindev
getoutdev

echo "Input device is $INDEV" >> $LOG
echo "Output device is $OUTDEV" >> $LOG

./midimsg --panic $OUTDEV
./testlb -i $INDEV -o $OUTDEV -t 1 2>>$LOG
showresult 1 "Sane input test"
unset WA
echo "Repeating 5 times test #2"
for R in 1 2 3 4 5; do
./testlb -i $INDEV -o $OUTDEV -t 2 2>>$LOG
showresult 2 "Synchronization test" || WA=-s
done
if [ "$WA" == "-s" ]; then
   echo "Syncronization workaround enabled"|tee -a $LOG
fi
#for R in 1 10 100 1000; do
for R in 1 10 100; do
	if [ $R == 1 ]; then
		echo "Unitary tests"|tee -a $LOG
	else
		echo "Repeating $R times each test"|tee -a $LOG
	fi

	./testlb -i $INDEV -o $OUTDEV -t 3 -r $R $WA 2>>$LOG
	showresult 3 "System exclusive test"

	./testlb -i $INDEV -o $OUTDEV -t 4 -r $R $WA 2>>$LOG
	showresult 4 "System common test"

	./testlb -i $INDEV -o $OUTDEV -t 5 -r $R $WA 2>>$LOG
	showresult 5 "Note on/off test"

	./testlb -i $INDEV -o $OUTDEV -t 6 -r $R $WA 2>>$LOG
	showresult 6 "Channel aftertouch test"

	./testlb -i $INDEV -o $OUTDEV -t 7 -r $R $WA 2>>$LOG
	showresult 7 "Control change test"

	./testlb -i $INDEV -o $OUTDEV -t 8 -r $R $WA 2>>$LOG
	showresult 8 "Pitch bender test"

	./testlb -i $INDEV -o $OUTDEV -t 9 -r $R $WA 2>>$LOG
	showresult 9 "Program change test"

	./testlb -i $INDEV -o $OUTDEV -t 10 -r $R $WA 2>>$LOG
	showresult 10 "Polyphonic aftertouch test"
done
