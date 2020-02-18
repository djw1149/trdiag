@comment[$Revision: 1.5 $]
@device(postscript)
@libraryfile(stable)
@style(fontfamily timesroman)
@style(fontscale 10)
@style(references=stdalphabetic)	
@make(report)
@standardtable(name t1, columns 2, firstcolumn center,
othercolumns flushleft, firstcolumnwidth 1.25in, boxed)
@PageFooting(left 'Token Ring Diagnosis', center '@value(date)', right 'David Waitzman')
@begin(titlepage)
@begin(titlebox)
@majorheading(An Undergraduate Project Report on
Token Ring Router Diagnosis)
@blankspace(.25in)
David Waitzman
Department of Electrical & Computer Engineering
Carnegie Mellon University

for 

Professor Roy Maxion
Department of Computer Science
Carnegie Mellon University

@value(date)

18-615 Section S, 12 Units

@end(titlebox)
@begin(text, fill on)
@majorheading(Abstract)
This paper describes the features added to the CMU router to support the
diagnosis of the IBM Token Ring, and the additional features added to the
@i[rdiag] program, used to request diagnosis of a router device.
@end(text)
@end(titlepage)

@newpage()
@chapter(Introduction)

The IBM Token ring is a 4 megabit per second ring network (physically a star
topology).  The protocol used is @i(ANSI 802.5)m also referred to as
@i(ISP/DP 8802/5).  Texas Instruments manufacturers a chip set the
implements the protocol, which is sold by IBM in a Token Ring Adapter Card,
for use in the PC/AT.

The CMU router, an IP router, is widely used on the Carnegie Mellon campus
for interconnecting the various network cables.  The router code runs on a
few computers: 68000 Multibus, IBM PC/AT, PDP-11@footnote(not actively
maintained), and the Vax 750@footnote(currently being developed).  The
router supports a wide variety of network devices, including: Interlan 10MB
ethernet, 3Com 10MB ethernet, 3MB ethernet, DEC DEUNA 10MB ethernet,
AppleTalk, proNET 10MB ring, Chicken Hawk ethernet, and the IBM Token Ring.
The devices are primarily interrupt driven, though polling is done for
certain functions.

As part of an earlier project, I added support to the router to diagnose the
Interlan 10MB ethernet device.  This was implemented by extending the router
control protocol (@i[rcp]) router information (@i[rinfo]) module to also
serve as a router diagnosis (@i[rdiag]) module.  A program, called
@i[rdiag], was created to provide a user interface to the diagnosis
functions.  This program runs on 4.2bsd and 4.3bsd Unix hosts.

For this project I have added functionality to the router to support a
subset of the diagnostics provided by the TI Token Ring chip set.  The
@i[rdiag] program was also extended to provide machine readable output.

@chapter(Diagnosis Functions)

The TI Token Ring chip set provides a number of diagnosis functions.  Some
are specified in the 802.5 protocol specification, others are unique to the
TI chip set.  The @u[TMS380 Adapter Chipset User's Guide], revision D,
produced by Texas Instruments, provides information on the chip set.
Section 3.4 provides information on frame formats in the Token Ring.

The diagnosis function provided by the router, for the Token Ring Adapter
Card, is to return the error log counters maintained by the device.  These
counters are described in section 3.11 (@b[Soft Error Counting and
Reporting]) and summarized on page 4-133 of the Chipset User's Guide.  There
are counters for the following errors:
@comment(@tabclear()
@tabset(1.25in, 2.25in))
@begin(description, leftmargin +2.5 in, indent -1.5 in, size -1)
@ux[@b[line]]@\The line error counter is incremented once per frame 
whenever:
1) a frame is repeated  or copied, or 2) the Error Detected Indicator (EDI)
@footnote(The EDI is set by a repeating adapter if it detects a CRC error or
a code violation.)@ 
is zero in the incoming frame, or 3) one of the following conditions exist:
@Begin(itemize)
A code violation exists between the starting delimiter (SDEL) and Ending
Delimiter (EDEL) of the frame.

A code violation exists in a token.

A Frame Check Sequence (FCS) error exists.
@End(itemize)

@ux[@b[burst]]@\The burst error counter is incremented when the adapter 
detects the absence of transitions for five half-bit times between SDEL and 
EDEL.  Only one Adapter detects the burst five condition, because the
Adapter that detects a burst four condition (four half-bit times without
transitions) conditions its transmitter to transmit idles if the burst five
condition is detected.

@ux[@b[ari/fci set]]@\The ARI (Address Recognized)/FCI (Frame Copied) set 
error counter is incremented when more than one AMP or SMP MAC frame is
received with ARI/FCI equal to zero without first receiving an intervening
AMP MAC frame.  This counter indicates that the upstream Adapter is unable
to set its ARI/FCI bits in a frame that it has copied.

@ux[@b[lost frame]]@\The lost frame error counter is incremented when an Adapter 
is in transmit (stripping) mode and fails to receive the end of the frame
it transmitted.

@ux[@b[receive congestion]]@\The receive congestion error counter is 
incremented when an Adapter in the repeat mode recognizes a frame addressed
to its specific address, but has no buffer space available to copy the frame.

@ux[@b[frame copied]]@\The frame copied error counter is incremented when an
Adapter in the receive/repeat mode recognizes a frame addressed to its
specific address, but finds the ARI bits not equal to 00 (possible line hit
or duplicated address).

@ux[@b[token]]@\The token error counter is only contained (incremented) in an
Active Monitor Adapter configuration.  It is incremented when the Active
monitor function detects an error with the token protocol as follows:
@Begin(itemize)
A token with a priority of non-zero and the MONITOR COUNT bit equals one.

A frame and the MONITOR COUNT equals one.

No token or frame is received within a 10 millisecond window.

The starting delimiter/token sequence has a code violation (in an area where
a code violation must not exist).
@End(itemize)

@ux[@b[dma bus]]@\The DMA bus error counter counts the occurances of DMA bus
errors which do not exceed the abort thresholds as specified in the Adapter
initialization parameters.

@ux[@b[dma parity]]@\The DMA Parity error counter counts the occurances of DMA 
parity errors which do not exceed the abort thresholds as specified in the 
Adapter initialization parameters.
@End(description)

Five reserved counters are also allocated, to bring the total number of
counters to fourteen.

Each counter is eight bits wide.  When a counter overflows (when its value
reaches 255), a @b[Ring Status] interrupt occurs with the @b[Counter
Overflow] bit set in the status description.  The counter that overflowed is
pegged at 255.  The counters can be read with the @b[Read Error Log]
command.  This command returns the values of all of the counters, and then
zeros all of the counters.

The following lists the order the counters are read out of the device with
the @b[Read Error Log] command:
@heading(Table 1: Counter Ordering)
@begin(t1)
@tableid(tt1)
@tableheading(immediate, rowformat t1columnheadings, line (@b[Counter Number]@\@b[Counter Name]))
0@\Line

1@\Reserved

2@\Burst

3@\ARI/FCI  

4@\Reserved

5@\Reserved

6@\LostFrame 

7@\Receive Congestion 

8@\Frame Copied

9@\Reserved

10@\Token

11@\Reserved

12@\DMA Bus  

13@\DMA Parity
@end()

Because the chip set counters are only eight bits wide, a way was needed to
maintain the total error count.  This implementation maintains a long
integer (thirty two bit wide) counter, updated by the router, that sums the
total errors for each chip set counter.  The long integer counters will be
updated upon the following two conditions:
@Begin(Itemize)
a @b[Ring Status] interrupt due to a @b[Counter Overflow] occurs, or

a request for the counter values is received by the router due to
a user running the @i[rdiag] program on a host computer.
@End(Itemize)

Because of the possible delay from when a counter overflows, pegs at 255 and
causes an interrupt, and when the interrupt is serviced, some errors counts
may be lost in the pegged counter.  I attempted to find a typical number of
errors lost in this case by performance testing.  

Long integer counters are also kept on the number of pegs that occur for
each adapter counter.  This will provide a way to estimate the real number
of errors that occurred.

@chapter(Implementation)
This chapter describes how the diagnosis function was added to the router
driver for the token ring chip set.

The device dependent data fields for the token ring (called tr) device had
three fourteen element arrays added to it: an array of bytes for the
adapter's counters, an array of longs for the total counters, and a array of
longs for the peg counters.  A semaphore,  @t[readerrlog_wait], was also
added, to support notifying the rdiag functions of @b[Read Error Log]
completion.  The device dependent data fields are replicated for each active
card in a running router (i.e. the counters are not shared by the token ring
cards).

The device driver, in the file @t[dev/tr.c], was slightly modified .  This
device driver operates at two priority levels: interrupt level (no
interrupts can be received), and user level (interrupts may be received).  A
@t[read_error_log] function, to be invoked in the interrupt priority level,
was created.  This sets up a read of the counters by issuing a @b[Read Error
Log] command to the token ring card.  The command requires a buffer to place
the counters into.  This buffer is contained in the device dependent data
for the relevant card.  When the read is completed, an @b[Command
Completion] interrupt occurs.  The interrupt is serviced by a routine,
@t[complete_read_error_log] which adds the eight bit counter values to the
long counter values, and increments the peg counts as needed.  The semaphore
@t[readerrlog_wait] is then set, to notify  routine in the user priority
level.

To handle user requests for the error counters, the @t[read_error_log]
function is called at the interrupt priority level.  Then the rdiag routine
goes to user level priority so that the @b[Command Completion Interrupt] can
be taken.  A spin loop is used to wait for the @t[read_error_log] to be
completed, by looking at the @t[readerrlog_wait] semaphore.  Once the
semaphore is set, the @i[rdiag] result packet with the updated counter
values is sent to the user program.  If the semaphore is not set in a
reasonable amount of time, the diagnosis fails.  This is reported back to the
user of the @i[rdiag] program.

Because the @b[Read Error Log] command is only valid for an open device, a
user request for the error counters will fail when the tr device is closed.
This can happen at any time during the lifetime of a router invocation,
either because the tr device has not opened yet (it takes about fifteen
seconds for the device to open when the router is first booted), or because
the tr device may close (temporarily or permanently) due to serious errors
on the ring.  The user interface program displays the message:
@t<"device is not ready for diagnosis- retry later">.

@chapter(Testing Methods and Results)
This chapter describes the methods and results of testing the diagnosis
functions in the router.  The test methods involve creating faults on the
ring and noting the reaction of the router.  This testing is being used
soley to determine if the @i[rdiag] code is capable of reading the error
counters correctly.  The chip set microcode actually determines,
categorizes, and counts errors.  Therefore, my testing must simply try to
obtain and report some number of errors.

A few procedures for creating faults were used:
@Begin(enumerate)
@i[Disconnecting and reconnecting the ring connector cable for an adapter.]

@i[Shorting the ring-]  There are essentially two wires in the ring that are
available via a cut-up wiring connector.
Simply shorting the ring wiring together causes errors.  

@i[Applying an external signal to the ring-]  If a function generator is
used to apply a signal to the ring, the ring notices errors for certain
types of inserted signals.  Attach the function generator ground to one wire,
and the function generator hot (signal) lead to the second wire.
@End(enumerate)

The first method creates relatively few errors.  Typically, a line error and
a token error are caused when the cable is disconnected.  When the cable is
reconnected, one or two frame copied errors are caused.  Note that the token
error is only counted by Active Monitors on the ring.  If the cable is left
off for about six seconds the adapter signals a @b[lobe fault], but this
does not cause the error counters to change any differently than for a
shorter disconnected period.  This test method shows the kind of errors that
could be expected on a ring due to computer installation.

The second method usually causes line, burst, lost frame, and frame copied
errors.  Because of the variable nature of this method, the number of errors
that occur per affected counter is relatively random, but usually one to
ten.  If the short is permanent, the ring shuts down.  This models a wire
fault in the ring cabling.

The third method usually causes line, burst, lost frame, and token errors.
Using a 10% duty cycle, 2V@-[p-p] square wave the following errors are noted
per two second interval (approximate counts):
@Begin(itemize)
At 10hz: 4 burst errors, 25 token errors

At 1khz: 50 line errors, 60 burst errors, 1 lost frame error, and 70 token
errors

At 10khz: 250 line errors, 300 burst errors, 1 lost frame error, and 80 token
errors

At 10khz and a 13% duty cycle or higher: the ring shuts down until the
signal is removed.  At lower duty cycles the ring may shut down a few
seconds at a time, but it comes back up.
@End(itemize)

With higher frequencies, the duty cycle must be low or the ring quickly
shuts down.  At very high frequencies (4mhz, the data rate), signal voltages
greater than ~.3V@-[p-p] will shut down the ring.  Lower voltage signals do
not effect the ring.

This third method of testing simulates noise on the ring.  I am not clear
what kind of signals best represent noise, so I vary many different
parameters.  If a better representation were available of the real world
failure modes, a token ring experiences, more useful tests could be
performed.

Earlier in this paper, a desire was expressed to count the number of errors
lost between a peg and when the error counters are read.  Due to the random
nature of the errors, this desire proved impossible to fufill.

@chapter(Modifications to @i[rdiag])
This chapter describes the modifications to the @i[rdiag] program made
during this project.  The follow features were added:
@Begin(itemize)
Ability to specify the @b[errs] (read error counters) diagnosis of the @b(tr)
(token ring device).

A machine readable output format which prints numeric data separated by
spaces.  All non-fatal error messages are sent to the standard output 
preceeded by a question mark.

A new way of specify options to @i[rdiag] has been created, using the C
library function @b[getopt]().

Ability to specify a repeat count (or infinite repetition) and a delay
between repetitions for diagnoses.

An X interface to @i[rdiag] that allows easy, but simple, router diagnosis.
This is in a program called @i[xfrdiag].
@End(itemize)

As a transistional phase, the new version of @i[rdiag], that has the new
option specification format, is called @i[nrdiag].  The @i[rdiag] program
still exists, but does not support the repeat count options.  @i[rdiag] will
be deleted eventually.

@section(Description of Output Formats)
@i[Nrdiag] now provides three different output formats.  This section
describe the output formats available with the @i[tr] device
@i[errs] diagnosis.

The @b[verbose] output format from @i[nrdiag] provides a list of the name of
the counter, the non-reserved error counter and the peg counter, with each
peg counter adjacent to the error counter that it applies too, i.e.
@display<
@i<	
	counter name    error counter   peg counter
	counter name    error counter   peg counter
	counter name    error counter   peg counter
	.
	.
	.>
@t<	
	line errors:	0, pegs:      0
	burst errors:	0, pegs:      0
        ari/fci errors:	0, pegs:      0
	.
	.
	.>
>

The @b[terse] output format provides a single line of output, with the card
number, a colon, all fourteen error counters (including the reserved
counters) and then the fourteen peg counters, i.e.
@display<
@i<card number:    14 error counters    14 peg counters>
@t<1: 494 0 4 0 0 0 401 1 0 0 15113 0 0 0 0 0 0 0 0 0 0 0 0 0 37 0 0 0>
>

The @b[machine readable] output format provides a single line of output,
with the card number, all fourteen error counters (including the
reserved counters) and then the fourteen peg counters, i.e.
@display<
@i<card number    14 error counters    14 peg counters>
@t<1 494 0 4 0 0 0 401 1 0 0 15113 0 0 0 0 0 0 0 0 0 0 0 0 0 37 0 0 0>
>

Error messages are primarily sent to the standard error output for the
@b[verbose] and @b[terse] formats, and to the standard output for the
@b[machine readable] formats.

@chapter(Conclusion)
This project has been successful, in terms of adding a simple token ring
diagnosis capability to the router.  There are additional diagnosis
capabilities that could be added, but were not provided by this project.
Better testing could make the output more meaningful.

The extensions to @i[rdiag] make it more useful for some applications.


@Appendix(Sample diagnosis output)
See @b[table 1] and @b[section 5.1] for an explanation of the output formats.

From the command @t<nrdiag -r128.2.251.238 -D tr -d errs -i -I2>
@footnote(Router: 128.2.251.238, Device type: tr, Diagnosis: errs, Infinite
repetition, Delay between repetitions: 2 seconds, Terse output)
@ @ selected output as the ring was repeatedly shorted:
@begin(verbatim)
1: 0 0 0 0 0 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 3 0 2 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 0 0 0 0 0 0 5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 8 0 7 0 0 0 2 0 0 0 104 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 0 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 17 0 9 0 0 0 2 0 0 0 130 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 0 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 21 0 9 0 0 0 3 0 0 0 156 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 0 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 23 0 10 0 0 0 3 0 0 0 193 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 1 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 24 0 11 0 0 0 3 0 0 0 222 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 1 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 24 0 11 0 0 0 3 0 0 0 227 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 2 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 30 0 11 0 0 0 3 0 0 0 240 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 3 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 75 0 14 0 0 0 3 0 0 0 275 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 3 0 0 0 0 0 7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 110 0 15 0 0 0 3 0 0 0 299 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: 4 0 0 0 0 0 14 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
2: 138 0 19 0 0 0 3 0 0 0 332 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
1: rdiag: diagnosis error- device is not ready for diagnosis- retry later
.
.	@b(the device shut down for a while)
.
@end(verbatim)
@newpage
Selected output from the command @t<nrdiag -r128.2.251.238 -D tr -d errs -v -R 2 -I 20>
@footnote(Router: 128.2.251.238, Device type: tr, Diagnosis: errs, 
Repetition count: 2, Delay between repetitions: 20 seconds, Verbose Output):

@begin(verbatim)
@b(initially:)
128.2.251.238:: Card: 1, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:      0, pegs:      0
                    burst errors:      0, pegs:      0
                  ari/fci errors:      0, pegs:      0
               lost frame errors:      0, pegs:      0
       receive congestion errors:      0, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:      0, pegs:      0
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
128.2.251.238:: Card: 2, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:      0, pegs:      0
                    burst errors:      0, pegs:      0
                  ari/fci errors:      0, pegs:      0
               lost frame errors:      0, pegs:      0
       receive congestion errors:      0, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:      0, pegs:      0
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
@b(after many shorts:)
128.2.251.238:: Card: 1, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:      6, pegs:      0
                    burst errors:      1, pegs:      0
                  ari/fci errors:      0, pegs:      0
               lost frame errors:     22, pegs:      0
       receive congestion errors:      0, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:      0, pegs:      0
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
128.2.251.238:: Card: 2, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:     83, pegs:      0
                    burst errors:     35, pegs:      0
                  ari/fci errors:      0, pegs:      0
               lost frame errors:      5, pegs:      0
       receive congestion errors:      0, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:    284, pegs:      1
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
@end(verbatim)

@newpage
After all of the final testing done for this paper, output from the command
@blankspace(.1cm)
@t<../vax/nrdiag -r128.2.251.238 -D tr -d errs -m>
@footnote(Router: 128.2.251.238, Device type: tr, Diagnosis: errs, No
repetition, Machine readable diagnosis):

@Begin(verbatim)
1 494 0 4 0 0 0 401 1 0 0 15113 0 0 0 0 0 0 0 0 0 0 0 0 0 37 0 0 0 
2 13259 0 10629 0 0 0 144 4 0 0 1636 0 0 0 13 0 29 0 0 0 0 0 0 0 3 0 0 0 
@End(verbatim)

With terse output (@t[-t]), the default:
@Begin(verbatim)
1 494 0 4 0 0 0 401 1 0 0 15113 0 0 0 0 0 0 0 0 0 0 0 0 0 37 0 0 0 
2 13259 0 10629 0 0 0 144 4 0 0 1636 0 0 0 13 0 29 0 0 0 0 0 0 0 3 0 0 0 
@End(verbatim)

With verbose output (@t[-v]):
@Begin(verbatim)
128.2.251.238:: Card: 1, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:    494, pegs:      0
                    burst errors:      4, pegs:      0
                  ari/fci errors:      0, pegs:      0
               lost frame errors:    401, pegs:      0
       receive congestion errors:      1, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:  15113, pegs:     37
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
128.2.251.238:: Card: 2, Diagnosis: errs, Device type: IBM Token Ring
                     line errors:  13259, pegs:     13
                    burst errors:  10629, pegs:     29
                  ari/fci errors:      0, pegs:      0
               lost frame errors:    144, pegs:      0
       receive congestion errors:      4, pegs:      0
             frame copied errors:      0, pegs:      0
              token error errors:   1636, pegs:      3
                  dma bus errors:      0, pegs:      0
               dma parity errors:      0, pegs:      0
@End(verbatim)

@Appendix(The @i[rdiag] man page)

@Appendix(Selected Portions of the @u[TMS380 Adapter Chipset User's Guide])
This appendix contains selected sections of the @u[TMS380 Adapter Chipset
User's Guide] that will be useful to understand this report.

@Appendix(Selected Router Code)
This appendix contains:
@Begin(Itemize)
The token ring device driver: @i[tr.c]

The token ring main header file: @i[tr.h]

The token ring internal register header file: @i[trreg.h]
@End(Itemize)


@Appendix(@i[Nrdiag] Code)
This appendix contains:
@Begin(Itemize)
The main code: @i[nrdiag.c]

The utility code: @i[nrd_utils.c]

The Token Ring diagnosis output code: @i[Dtr.c]

A header file: @i[rdiag.h]
@End(Itemize)
