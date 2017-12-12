set ns [new Simulator]

#open nam trace file
set nf [open output.nam w]
$ns namtrace-all $nf

#take in input parameters
#Report error when input is not correct
if { $argc != 2 } {
        puts "Invalid input!"
    }
set flavor [lindex $argv 0]
set input_case [lindex $argv 1]
if {$input_case > 3 || $input_case < 1} { 
	puts "Invalid input case $input_case" 
   	exit
}
global flav, delay
set delay 0
switch $input_case {
	global delay
	1 {set delay "12.5ms"}
	2 {set delay "20ms"}
	3 {set delay "27.5ms"}
}
if {$flavor == "SACK"} {
	set flav "Sack1"
} elseif {$flavor == "VEGAS"} {
	set flav "Vegas"
} else {
	puts "Invalid TCP Flavor $flavor"
	exit
}

#create 6 nodes
set source1 [$ns node]
set source2 [$ns node]
set router1 [$ns node]
set router2 [$ns node]
set receiver1 [$ns node]
set receiver2 [$ns node]

#create TCP connection
set tcp1 [new Agent/TCP/$flav]
$ns attach-agent $source1 $tcp1
set tcp2 [new Agent/TCP/$flav]
$ns attach-agent $source2 $tcp2

#create links between nodes with the input delay
$ns duplex-link $source1 $router1 10Mb 5ms DropTail
$ns duplex-link $source2 $router1 10Mb $delay DropTail
$ns duplex-link $router1 $router2 1Mb 5ms DropTail
$ns duplex-link $router2 $receiver1 10Mb 5ms DropTail
$ns duplex-link $router2 $receiver2 10Mb $delay DropTail

$ns duplex-link-op $source1 $router1 orient right-down
$ns duplex-link-op $source2 $router1 orient right-up
$ns duplex-link-op $router1 $router2 orient right
$ns duplex-link-op $router2 $receiver1 orient right-up
$ns duplex-link-op $router2 $receiver2 orient right-down


set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1

set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2


set null1 [new Agent/TCPSink] 
$ns attach-agent $receiver1 $null1

set null2 [new Agent/TCPSink] 
$ns attach-agent $receiver2 $null2


$ns connect $tcp1 $null1
$ns connect $tcp2 $null2

#schedule events 
#starts recording at 0 and finish at 400
$ns at 0 "record"
$ns at 0 "$ftp1 start"
$ns at 400 "$ftp1 stop"

$ns at 0 "$ftp2 start"
$ns at 400 "$ftp2 stop"

$ns at 400 "finish"

set testfile1 [open "file-S1.tr" w]
$ns trace-queue  $source1  $router1  $testfile1

set testfile2 [open "file-S2.tr" w]
$ns trace-queue  $source2  $router1  $testfile2
proc finish {} {
    global ns nf sum1 sum2 cnt
	$ns flush-trace
	close $nf
	exec nam -a output.nam &
	puts "Avgerage throughput for source1=[expr $sum1/$cnt] MBits/sec\n"
	puts "Avgerage throughput for source2=[expr $sum2/$cnt] MBits/sec\n"
    exit 0
}

set file1 [open out1.tr w]
set file2 [open out2.tr w]

proc record {} {
        global null1 null2 file1 file2 sum1 sum2 cnt
        set ns [Simulator instance]
        set time 0.5
		set cnt 0
		set sum1 0
		set sum2 0
        set bw1 [$null1 set bytes_]
        set bw2 [$null2 set bytes_]
        set now [$ns now]
        puts $file1 "$now [expr $bw1/$time*8/1000000]"
        puts $file2 "$now [expr $bw2/$time*8/1000000]"
		set sum1 [expr $sum1 + $bw1/$time*8/1000000]
		set sum2 [expr $sum2 + $bw2/$time*8/1000000]
		set cnt [expr $cnt+1]
        #Reset the bytes_ values on sinks
        $null1 set bytes_ 0
        $null2 set bytes_ 0
        #Re-schedule the procedure
        $ns at [expr $now+$time] "record"
}

$ns run