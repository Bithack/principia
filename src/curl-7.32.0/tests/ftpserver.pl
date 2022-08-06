#!/usr/bin/env perl
#***************************************************************************
#                                  _   _ ____  _
#  Project                     ___| | | |  _ \| |
#                             / __| | | | |_) | |
#                            | (__| |_| |  _ <| |___
#                             \___|\___/|_| \_\_____|
#
# Copyright (C) 1998 - 2013, Daniel Stenberg, <daniel@haxx.se>, et al.
#
# This software is licensed as described in the file COPYING, which
# you should have received as part of this distribution. The terms
# are also available at http://curl.haxx.se/docs/copyright.html.
#
# You may opt to use, copy, modify, merge, publish, distribute and/or sell
# copies of the Software, and permit persons to whom the Software is
# furnished to do so, under the terms of the COPYING file.
#
# This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
# KIND, either express or implied.
#
###########################################################################

# This is a server designed for the curl test suite.
#
# In December 2009 we started remaking the server to support more protocols
# that are similar in spirit. Like POP3, IMAP and SMTP in addition to the FTP
# it already supported since a long time. Note that it still only supports one
# protocol per invoke. You need to start multiple servers to support multiple
# protocols simultaneously.
#
# It is meant to exercise curl, it is not meant to be a fully working
# or even very standard compliant server.
#
# You may optionally specify port on the command line, otherwise it'll
# default to port 8921.
#
# All socket/network/TCP related stuff is done by the 'sockfilt' program.
#

BEGIN {
    push(@INC, $ENV{'srcdir'}) if(defined $ENV{'srcdir'});
    push(@INC, ".");
    # sub second timestamping needs Time::HiRes
    eval {
        no warnings "all";
        require Time::HiRes;
        import  Time::HiRes qw( gettimeofday );
    }
}

use strict;
use warnings;
use IPC::Open2;

require "getpart.pm";
require "ftp.pm";
require "directories.pm";

use serverhelp qw(
    servername_str
    server_pidfilename
    server_logfilename
    mainsockf_pidfilename
    mainsockf_logfilename
    datasockf_pidfilename
    datasockf_logfilename
    );

#**********************************************************************
# global vars...
#
my $verbose = 0;    # set to 1 for debugging
my $idstr = "";     # server instance string
my $idnum = 1;      # server instance number
my $ipvnum = 4;     # server IPv number (4 or 6)
my $proto = 'ftp';  # default server protocol
my $srcdir;         # directory where ftpserver.pl is located
my $srvrname;       # server name for presentation purposes

my $path   = '.';
my $logdir = $path .'/log';

#**********************************************************************
# global vars used for server address and primary listener port
#
my $port = 8921;               # default primary listener port
my $listenaddr = '127.0.0.1';  # default address for listener port

#**********************************************************************
# global vars used for file names
#
my $pidfile;            # server pid file name
my $logfile;            # server log file name
my $mainsockf_pidfile;  # pid file for primary connection sockfilt process
my $mainsockf_logfile;  # log file for primary connection sockfilt process
my $datasockf_pidfile;  # pid file for secondary connection sockfilt process
my $datasockf_logfile;  # log file for secondary connection sockfilt process

#**********************************************************************
# global vars used for server logs advisor read lock handling
#
my $SERVERLOGS_LOCK = 'log/serverlogs.lock';
my $serverlogslocked = 0;

#**********************************************************************
# global vars used for child processes PID tracking
#
my $sfpid;        # PID for primary connection sockfilt process
my $slavepid;     # PID for secondary connection sockfilt process

#**********************************************************************
# global typeglob filehandle vars to read/write from/to sockfilters
#
local *SFREAD;    # used to read from primary connection
local *SFWRITE;   # used to write to primary connection
local *DREAD;     # used to read from secondary connection
local *DWRITE;    # used to write to secondary connection

my $sockfilt_timeout = 5;  # default timeout for sockfilter eXsysreads

#**********************************************************************
# global vars which depend on server protocol selection
#
my %commandfunc;  # protocol command specific function callbacks
my %displaytext;  # text returned to client before callback runs

#**********************************************************************
# global vars customized for each test from the server commands file
#
my $ctrldelay;     # set if server should throttle ctrl stream
my $datadelay;     # set if server should throttle data stream
my $retrweirdo;    # set if ftp server should use RETRWEIRDO
my $retrnosize;    # set if ftp server should use RETRNOSIZE
my $pasvbadip;     # set if ftp server should use PASVBADIP
my $nosave;        # set if ftp server should not save uploaded data
my $nodataconn;    # set if ftp srvr doesn't establish or accepts data channel
my $nodataconn425; # set if ftp srvr doesn't establish data ch and replies 425
my $nodataconn421; # set if ftp srvr doesn't establish data ch and replies 421
my $nodataconn150; # set if ftp srvr doesn't establish data ch and replies 150
my $support_capa;  # set if server supports capability command
my $support_auth;  # set if server supports authentication command
my %customreply;   #
my %customcount;   #
my %delayreply;    #

#**********************************************************************
# global variables for to test ftp wildcardmatching or other test that
# need flexible LIST responses.. and corresponding files.
# $ftptargetdir is keeping the fake "name" of LIST directory.
#
my $ftplistparserstate;
my $ftptargetdir;

#**********************************************************************
# global variables used when running a ftp server to keep state info
# relative to the secondary or data sockfilt process. Values of these
# variables should only be modified using datasockf_state() sub, given
# that they are closely related and relationship is a bit awkward.
#
my $datasockf_state = 'STOPPED'; # see datasockf_state() sub
my $datasockf_mode = 'none';     # ['none','active','passive']
my $datasockf_runs = 'no';       # ['no','yes']
my $datasockf_conn = 'no';       # ['no','yes']

#**********************************************************************
# global vars used for signal handling
#
my $got_exit_signal = 0; # set if program should finish execution ASAP
my $exit_signal;         # first signal handled in exit_signal_handler

#**********************************************************************
# exit_signal_handler will be triggered to indicate that the program
# should finish its execution in a controlled way as soon as possible.
# For now, program will also terminate from within this handler.
#
sub exit_signal_handler {
    my $signame = shift;
    # For now, simply mimic old behavior.
    killsockfilters($proto, $ipvnum, $idnum, $verbose);
    unlink($pidfile);
    if($serverlogslocked) {
        $serverlogslocked = 0;
        clear_advisor_read_lock($SERVERLOGS_LOCK);
    }
    exit;
}

#**********************************************************************
# logmsg is general message logging subroutine for our test servers.
#
sub logmsg {
    my $now;
    # sub second timestamping needs Time::HiRes
    if($Time::HiRes::VERSION) {
        my ($seconds, $usec) = gettimeofday();
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
            localtime($seconds);
        $now = sprintf("%02d:%02d:%02d.%06d ", $hour, $min, $sec, $usec);
    }
    else {
        my $seconds = time();
        my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) =
            localtime($seconds);
        $now = sprintf("%02d:%02d:%02d ", $hour, $min, $sec);
    }
    if(open(LOGFILEFH, ">>$logfile")) {
        print LOGFILEFH $now;
        print LOGFILEFH @_;
        close(LOGFILEFH);
    }
}

sub ftpmsg {
  # append to the server.input file
  open(INPUT, ">>log/server$idstr.input") ||
    logmsg "failed to open log/server$idstr.input\n";

  print INPUT @_;
  close(INPUT);

  # use this, open->print->close system only to make the file
  # open as little as possible, to make the test suite run
  # better on windows/cygwin
}

#**********************************************************************
# eXsysread is a wrapper around perl's sysread() function. This will
# repeat the call to sysread() until it has actually read the complete
# number of requested bytes or an unrecoverable condition occurs.
# On success returns a positive value, the number of bytes requested.
# On failure or timeout returns zero.
#
sub eXsysread {
    my $FH      = shift;
    my $scalar  = shift;
    my $nbytes  = shift;
    my $timeout = shift; # A zero timeout disables eXsysread() time limit
    #
    my $time_limited = 0;
    my $timeout_rest = 0;
    my $start_time = 0;
    my $nread  = 0;
    my $rc;

    $$scalar = "";

    if((not defined $nbytes) || ($nbytes < 1)) {
        logmsg "Error: eXsysread() failure: " .
               "length argument must be positive\n";
        return 0;
    }
    if((not defined $timeout) || ($timeout < 0)) {
        logmsg "Error: eXsysread() failure: " .
               "timeout argument must be zero or positive\n";
        return 0;
    }
    if($timeout > 0) {
        # caller sets eXsysread() time limit
        $time_limited = 1;
        $timeout_rest = $timeout;
        $start_time = int(time());
    }

    while($nread < $nbytes) {
        if($time_limited) {
            eval {
                local $SIG{ALRM} = sub { die "alarm\n"; };
                alarm $timeout_rest;
                $rc = sysread($FH, $$scalar, $nbytes - $nread, $nread);
                alarm 0;
            };
            $timeout_rest = $timeout - (int(time()) - $start_time);
            if($timeout_rest < 1) {
                logmsg "Error: eXsysread() failure: timed out\n";
                return 0;
            }
        }
        else {
            $rc = sysread($FH, $$scalar, $nbytes - $nread, $nread);
        }
        if($got_exit_signal) {
            logmsg "Error: eXsysread() failure: signalled to die\n";
            return 0;
        }
        if(not defined $rc) {
            if($!{EINTR}) {
                logmsg "Warning: retrying sysread() interrupted system call\n";
                next;
            }
            if($!{EAGAIN}) {
                logmsg "Warning: retrying sysread() due to EAGAIN\n";
                next;
            }
            if($!{EWOULDBLOCK}) {
                logmsg "Warning: retrying sysread() due to EWOULDBLOCK\n";
                next;
            }
            logmsg "Error: sysread() failure: $!\n";
            return 0;
        }
        if($rc < 0) {
            logmsg "Error: sysread() failure: returned negative value $rc\n";
            return 0;
        }
        if($rc == 0) {
            logmsg "Error: sysread() failure: read zero bytes\n";
            return 0;
        }
        $nread += $rc;
    }
    return $nread;
}

#**********************************************************************
# read_mainsockf attempts to read the given amount of output from the
# sockfilter which is in use for the main or primary connection. This
# reads untranslated sockfilt lingo which may hold data read from the
# main or primary socket. On success returns 1, otherwise zero.
#
sub read_mainsockf {
    my $scalar  = shift;
    my $nbytes  = shift;
    my $timeout = shift; # Optional argument, if zero blocks indefinitively
    my $FH = \*SFREAD;

    if(not defined $timeout) {
        $timeout = $sockfilt_timeout + ($nbytes >> 12);
    }
    if(eXsysread($FH, $scalar, $nbytes, $timeout) != $nbytes) {
        my ($fcaller, $lcaller) = (caller)[1,2];
        logmsg "Error: read_mainsockf() failure at $fcaller " .
               "line $lcaller. Due to eXsysread() failure\n";
        return 0;
    }
    return 1;
}

#**********************************************************************
# read_datasockf attempts to read the given amount of output from the
# sockfilter which is in use for the data or secondary connection. This
# reads untranslated sockfilt lingo which may hold data read from the
# data or secondary socket. On success returns 1, otherwise zero.
#
sub read_datasockf {
    my $scalar = shift;
    my $nbytes = shift;
    my $timeout = shift; # Optional argument, if zero blocks indefinitively
    my $FH = \*DREAD;

    if(not defined $timeout) {
        $timeout = $sockfilt_timeout + ($nbytes >> 12);
    }
    if(eXsysread($FH, $scalar, $nbytes, $timeout) != $nbytes) {
        my ($fcaller, $lcaller) = (caller)[1,2];
        logmsg "Error: read_datasockf() failure at $fcaller " .
               "line $lcaller. Due to eXsysread() failure\n";
        return 0;
    }
    return 1;
}

sub sysread_or_die {
    my $FH     = shift;
    my $scalar = shift;
    my $length = shift;
    my $fcaller;
    my $lcaller;
    my $result;

    $result = sysread($$FH, $$scalar, $length);

    if(not defined $result) {
        ($fcaller, $lcaller) = (caller)[1,2];
        logmsg "Failed to read input\n";
        logmsg "Error: $srvrname server, sysread error: $!\n";
        logmsg "Exited from sysread_or_die() at $fcaller " .
               "line $lcaller. $srvrname server, sysread error: $!\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose);
        unlink($pidfile);
        if($serverlogslocked) {
            $serverlogslocked = 0;
            clear_advisor_read_lock($SERVERLOGS_LOCK);
        }
        exit;
    }
    elsif($result == 0) {
        ($fcaller, $lcaller) = (caller)[1,2];
        logmsg "Failed to read input\n";
        logmsg "Error: $srvrname server, read zero\n";
        logmsg "Exited from sysread_or_die() at $fcaller " .
               "line $lcaller. $srvrname server, read zero\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose);
        unlink($pidfile);
        if($serverlogslocked) {
            $serverlogslocked = 0;
            clear_advisor_read_lock($SERVERLOGS_LOCK);
        }
        exit;
    }

    return $result;
}

sub startsf {
    my $mainsockfcmd = "./server/sockfilt " .
        "--ipv$ipvnum --port $port " .
        "--pidfile \"$mainsockf_pidfile\" " .
        "--logfile \"$mainsockf_logfile\"";
    $sfpid = open2(*SFREAD, *SFWRITE, $mainsockfcmd);

    print STDERR "$mainsockfcmd\n" if($verbose);

    print SFWRITE "PING\n";
    my $pong;
    sysread_or_die(\*SFREAD, \$pong, 5);

    if($pong !~ /^PONG/) {
        logmsg "Failed sockfilt command: $mainsockfcmd\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose);
        unlink($pidfile);
        if($serverlogslocked) {
            $serverlogslocked = 0;
            clear_advisor_read_lock($SERVERLOGS_LOCK);
        }
        die "Failed to start sockfilt!";
    }
}


sub sockfilt {
    my $l;
    foreach $l (@_) {
        printf SFWRITE "DATA\n%04x\n", length($l);
        print SFWRITE $l;
    }
}


sub sockfiltsecondary {
    my $l;
    foreach $l (@_) {
        printf DWRITE "DATA\n%04x\n", length($l);
        print DWRITE $l;
    }
}


# Send data to the client on the control stream, which happens to be plain
# stdout.

sub sendcontrol {
    if(!$ctrldelay) {
        # spit it all out at once
        sockfilt @_;
    }
    else {
        my $a = join("", @_);
        my @a = split("", $a);

        for(@a) {
            sockfilt $_;
            select(undef, undef, undef, 0.01);
        }
    }
    my $log;
    foreach $log (@_) {
        my $l = $log;
        $l =~ s/\r/[CR]/g;
        $l =~ s/\n/[LF]/g;
        logmsg "> \"$l\"\n";
    }
}

#**********************************************************************
# Send data to the FTP client on the data stream when data connection
# is actually established. Given that this sub should only be called
# when a data connection is supposed to be established, calling this
# without a data connection is an indication of weak logic somewhere.
#
sub senddata {
    my $l;
    if($datasockf_conn eq 'no') {
        logmsg "WARNING: Detected data sending attempt without DATA channel\n";
        foreach $l (@_) {
            logmsg "WARNING: Data swallowed: $l\n"
        }
        return;
    }
    foreach $l (@_) {
      if(!$datadelay) {
        # spit it all out at once
        sockfiltsecondary $l;
      }
      else {
          # pause between each byte
          for (split(//,$l)) {
              sockfiltsecondary $_;
              select(undef, undef, undef, 0.01);
          }
      }
    }
}

#**********************************************************************
# protocolsetup initializes the 'displaytext' and 'commandfunc' hashes
# for the given protocol. References to protocol command callbacks are
# stored in 'commandfunc' hash, and text which will be returned to the
# client before the command callback runs is stored in 'displaytext'.
#
sub protocolsetup {
    my $proto = $_[0];

    if($proto eq 'ftp') {
        %commandfunc = (
            'PORT' => \&PORT_ftp,
            'EPRT' => \&PORT_ftp,
            'LIST' => \&LIST_ftp,
            'NLST' => \&NLST_ftp,
            'PASV' => \&PASV_ftp,
            'CWD'  => \&CWD_ftp,
            'PWD'  => \&PWD_ftp,
            'EPSV' => \&PASV_ftp,
            'RETR' => \&RETR_ftp,
            'SIZE' => \&SIZE_ftp,
            'REST' => \&REST_ftp,
            'STOR' => \&STOR_ftp,
            'APPE' => \&STOR_ftp, # append looks like upload
            'MDTM' => \&MDTM_ftp,
        );
        %displaytext = (
            'USER' => '331 We are happy you popped in!',
            'PASS' => '230 Welcome you silly person',
            'PORT' => '200 You said PORT - I say FINE',
            'TYPE' => '200 I modify TYPE as you wanted',
            'LIST' => '150 here comes a directory',
            'NLST' => '150 here comes a directory',
            'CWD'  => '250 CWD command successful.',
            'SYST' => '215 UNIX Type: L8', # just fake something
            'QUIT' => '221 bye bye baby', # just reply something
            'MKD'  => '257 Created your requested directory',
            'REST' => '350 Yeah yeah we set it there for you',
            'DELE' => '200 OK OK OK whatever you say',
            'RNFR' => '350 Received your order. Please provide more',
            'RNTO' => '250 Ok, thanks. File renaming completed.',
            'NOOP' => '200 Yes, I\'m very good at doing nothing.',
            'PBSZ' => '500 PBSZ not implemented',
            'PROT' => '500 PROT not implemented',
            'welcome' => join("",
            '220-        _   _ ____  _     '."\r\n",
            '220-    ___| | | |  _ \| |    '."\r\n",
            '220-   / __| | | | |_) | |    '."\r\n",
            '220-  | (__| |_| |  _ <| |___ '."\r\n",
            '220    \___|\___/|_| \_\_____|'."\r\n")
        );
    }
    elsif($proto eq 'pop3') {
        %commandfunc = (
            'CAPA' => \&CAPA_pop3,
            'AUTH' => \&AUTH_pop3,
            'RETR' => \&RETR_pop3,
            'LIST' => \&LIST_pop3,
        );
        %displaytext = (
            'USER' => '+OK We are happy you popped in!',
            'PASS' => '+OK Access granted',
            'QUIT' => '+OK byebye',
            'welcome' => join("",
            '        _   _ ____  _     '."\r\n",
            '    ___| | | |  _ \| |    '."\r\n",
            '   / __| | | | |_) | |    '."\r\n",
            '  | (__| |_| |  _ <| |___ '."\r\n",
            '   \___|\___/|_| \_\_____|'."\r\n",
            '+OK cURL POP3 server ready to serve'."\r\n")
        );
    }
    elsif($proto eq 'imap') {
        %commandfunc = (
            'APPEND' => \&APPEND_imap,
            'CAPABILITY' => \&CAPABILITY_imap,
            'EXAMINE' => \&EXAMINE_imap,
            'FETCH'  => \&FETCH_imap,
            'LIST'   => \&LIST_imap,
            'LOGOUT'   => \&LOGOUT_imap,
            'SELECT' => \&SELECT_imap,
            'STATUS'  => \&STATUS_imap,
            'STORE'  => \&STORE_imap
        );
        %displaytext = (
            'LOGIN'  => ' OK LOGIN completed',
            'welcome' => join("",
            '        _   _ ____  _     '."\r\n",
            '    ___| | | |  _ \| |    '."\r\n",
            '   / __| | | | |_) | |    '."\r\n",
            '  | (__| |_| |  _ <| |___ '."\r\n",
            '   \___|\___/|_| \_\_____|'."\r\n",
            '* OK cURL IMAP server ready to serve'."\r\n")
        );
    }
    elsif($proto eq 'smtp') {
        %commandfunc = (
            'DATA' => \&DATA_smtp,
            'RCPT' => \&RCPT_smtp,
        );
        %displaytext = (
            'EHLO' => "250-SIZE\r\n250 Welcome visitor, stay a while staaaaaay forever",
            'MAIL' => '200 Note taken',
            'RCPT' => '200 Receivers accepted',
            'QUIT' => '200 byebye',
            'welcome' => join("",
            '220-        _   _ ____  _     '."\r\n",
            '220-    ___| | | |  _ \| |    '."\r\n",
            '220-   / __| | | | |_) | |    '."\r\n",
            '220-  | (__| |_| |  _ <| |___ '."\r\n",
            '220    \___|\___/|_| \_\_____|'."\r\n")
        );
    }
}

sub close_dataconn {
    my ($closed)=@_; # non-zero if already disconnected

    my $datapid = processexists($datasockf_pidfile);

    logmsg "=====> Closing $datasockf_mode DATA connection...\n";

    if(!$closed) {
        if($datapid > 0) {
            logmsg "Server disconnects $datasockf_mode DATA connection\n";
            print DWRITE "DISC\n";
            my $i;
            sysread DREAD, $i, 5;
        }
        else {
            logmsg "Server finds $datasockf_mode DATA connection already ".
                   "disconnected\n";
        }
    }
    else {
        logmsg "Server knows $datasockf_mode DATA connection is already ".
               "disconnected\n";
    }

    if($datapid > 0) {
        print DWRITE "QUIT\n";
        waitpid($datapid, 0);
        unlink($datasockf_pidfile) if(-f $datasockf_pidfile);
        logmsg "DATA sockfilt for $datasockf_mode data channel quits ".
               "(pid $datapid)\n";
    }
    else {
        logmsg "DATA sockfilt for $datasockf_mode data channel already ".
               "dead\n";
    }

    logmsg "=====> Closed $datasockf_mode DATA connection\n";

    datasockf_state('STOPPED');
}

################
################ SMTP commands
################

# what set by "RCPT"
my $smtp_rcpt;

sub DATA_smtp {
    my $testno;

    if($smtp_rcpt =~ /^TO:(.*)/) {
        $testno = $1;
    }
    else {
        return; # failure
    }

    if($testno eq "<verifiedserver>") {
        sendcontrol "554 WE ROOLZ: $$\r\n";
        return 0; # don't wait for data now
    }
    else {
        $testno =~ s/^([^0-9]*)([0-9]+).*/$2/;
        sendcontrol "354 Show me the mail\r\n";
    }

    logmsg "===> rcpt $testno was $smtp_rcpt\n";

    my $filename = "log/upload.$testno";

    logmsg "Store test number $testno in $filename\n";

    open(FILE, ">$filename") ||
        return 0; # failed to open output

    my $line;
    my $ulsize=0;
    my $disc=0;
    my $raw;
    while (5 == (sysread \*SFREAD, $line, 5)) {
        if($line eq "DATA\n") {
            my $i;
            my $eob;
            sysread \*SFREAD, $i, 5;

            my $size = 0;
            if($i =~ /^([0-9a-fA-F]{4})\n/) {
                $size = hex($1);
            }

            read_mainsockf(\$line, $size);

            $ulsize += $size;
            print FILE $line if(!$nosave);

            $raw .= $line;
            if($raw =~ /\x0d\x0a\x2e\x0d\x0a/) {
                # end of data marker!
                $eob = 1;
            }
            logmsg "> Appending $size bytes to file\n";
            if($eob) {
                logmsg "Found SMTP EOB marker\n";
                last;
            }
        }
        elsif($line eq "DISC\n") {
            # disconnect!
            $disc=1;
            last;
        }
        else {
            logmsg "No support for: $line";
            last;
        }
    }
    if($nosave) {
        print FILE "$ulsize bytes would've been stored here\n";
    }
    close(FILE);
    sendcontrol "250 OK, data received!\r\n";
    logmsg "received $ulsize bytes upload\n";

}

sub RCPT_smtp {
    my ($args) = @_;

    $smtp_rcpt = $args;
}

################
################ IMAP commands
################

# global to allow the command functions to read it
my $cmdid;

# what was picked by SELECT
my $selected;

# Any IMAP parameter can come in escaped and in double quotes.
# This function is dumb (so far) and just removes the quotes if present.
sub fix_imap_params {
    foreach (@_) {
        $_ = $1 if /^"(.*)"$/;
    }
}

sub CAPABILITY_imap {
    my ($testno) = @_;
    my $data;

    if(!$support_capa) {
        sendcontrol "$cmdid BAD Command\r\n";
    }
    else {
        $data = "* CAPABILITY IMAP4";
        if($support_auth) {
            $data .= " AUTH=UNKNOWN";
        }
        $data .= " pingpong test server\r\n";

        sendcontrol $data;
        sendcontrol "$cmdid OK CAPABILITY completed\r\n";
    }

    return 0;
}

sub SELECT_imap {
    my ($testno) = @_;
    fix_imap_params($testno);

    logmsg "SELECT_imap got test $testno\n";

    # Example from RFC 3501, 6.3.1. SELECT Command
    sendcontrol "* 172 EXISTS\r\n";
    sendcontrol "* 1 RECENT\r\n";
    sendcontrol "* OK [UNSEEN 12] Message 12 is first unseen\r\n";
    sendcontrol "* OK [UIDVALIDITY 3857529045] UIDs valid\r\n";
    sendcontrol "* OK [UIDNEXT 4392] Predicted next UID\r\n";
    sendcontrol "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n";
    sendcontrol "* OK [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n";
    sendcontrol "$cmdid OK [READ-WRITE] SELECT completed\r\n";

    $selected = $testno;

    return 0;
}

sub FETCH_imap {
    my ($args) = @_;
    my ($uid, $how) = split(/ /, $args, 2);
    my @data;
    my $size;
    fix_imap_params($uid, $how);

    logmsg "FETCH_imap got $args\n";

    if($selected eq "verifiedserver") {
        # this is the secret command that verifies that this actually is
        # the curl test server
        my $response = "WE ROOLZ: $$\r\n";
        if($verbose) {
            print STDERR "FTPD: We returned proof we are the test server\n";
        }
        $data[0] = $response;
        logmsg "return proof we are we\n";
    }
    else {
        logmsg "retrieve a mail\n";

        my $testno = $selected;
        $testno =~ s/^([^0-9]*)//;
        my $testpart = "";
        if ($testno > 10000) {
            $testpart = $testno % 10000;
            $testno = int($testno / 10000);
        }

        # send mail content
        loadtest("$srcdir/data/test$testno");

        @data = getpart("reply", "data$testpart");
    }

    for (@data) {
        $size += length($_);
    }

    sendcontrol "* $uid FETCH ($how {$size}\r\n";

    for my $d (@data) {
        sendcontrol $d;
    }

    sendcontrol ")\r\n";
    sendcontrol "$cmdid OK FETCH completed\r\n";

    return 0;
}

sub APPEND_imap {
    my ($args) = @_;

    logmsg "APPEND_imap got $args\r\n";

    $args =~ /^([^ ]+) [^{]*\{(\d+)\}$/;
    my ($folder, $size) = ($1, $2);
    fix_imap_params($folder);

    sendcontrol "+ Ready for literal data\r\n";

    my $testno = $folder;
    my $filename = "log/upload.$testno";

    logmsg "Store test number $testno in $filename\n";

    open(FILE, ">$filename") ||
        return 0; # failed to open output

    my $received = 0;
    my $line;
    while(5 == (sysread \*SFREAD, $line, 5)) {
        if($line eq "DATA\n") {
            sysread \*SFREAD, $line, 5;

            my $chunksize = 0;
            if($line =~ /^([0-9a-fA-F]{4})\n/) {
                $chunksize = hex($1);
            }

            read_mainsockf(\$line, $chunksize);

            my $left = $size - $received;
            my $datasize = ($left > $chunksize) ? $chunksize : $left;

            if($datasize > 0) {
                logmsg "> Appending $datasize bytes to file\n";
                print FILE substr($line, 0, $datasize) if(!$nosave);
                $line = substr($line, $datasize);

                $received += $datasize;
                if($received == $size) {
                    logmsg "Received all data, waiting for final CRLF.\n";
                }
            }

            if($received == $size && $line eq "\r\n") {
                last;
            }
        }
        elsif($line eq "DISC\n") {
            logmsg "Unexpected disconnect!\n";
            last;
        }
        else {
            logmsg "No support for: $line";
            last;
        }
    }

    if($nosave) {
        print FILE "$size bytes would've been stored here\n";
    }
    close(FILE);

    logmsg "received $size bytes upload\n";

    sendcontrol "$cmdid OK APPEND completed\r\n";

    return 0;
}

sub STORE_imap {
    my ($args) = @_;
    my ($uid, $what) = split(/ /, $args, 2);
    fix_imap_params($uid);

    logmsg "STORE_imap got $args\n";

    sendcontrol "* $uid FETCH (FLAGS (\\Seen \\Deleted))\r\n";
    sendcontrol "$cmdid OK STORE completed\r\n";

    return 0;
}

sub LIST_imap {
    my ($args) = @_;
    my ($reference, $mailbox) = split(/ /, $args, 2);
    my @data;
    fix_imap_params($reference, $mailbox);

    logmsg "LIST_imap got $args\n";

    if ($reference eq "verifiedserver") {
         # this is the secret command that verifies that this actually is
         # the curl test server
         @data = ("* LIST () \"/\" \"WE ROOLZ: $$\"\r\n");
         if($verbose) {
             print STDERR "FTPD: We returned proof we are the test server\n";
         }
         logmsg "return proof we are we\n";
    }
    else {
        my $testno = $reference;
        $testno =~ s/^([^0-9]*)//;
        my $testpart = "";
        if ($testno > 10000) {
            $testpart = $testno % 10000;
            $testno = int($testno / 10000);
        }
    
        loadtest("$srcdir/data/test$testno");

        @data = getpart("reply", "data$testpart");
    }

    for my $d (@data) {
        sendcontrol $d;
    }

    sendcontrol "$cmdid OK LIST Completed\r\n";

    return 0;
}

sub EXAMINE_imap {
    my ($testno) = @_;
    fix_imap_params($testno);

    logmsg "EXAMINE_imap got test $testno\n";

    # Example from RFC 3501, 6.3.2. EXAMINE Command
    sendcontrol "* 17 EXISTS\r\n";
    sendcontrol "* 2 RECENT\r\n";
    sendcontrol "* OK [UNSEEN 8] Message 8 is first unseen\r\n";
    sendcontrol "* OK [UIDVALIDITY 3857529045] UIDs valid\r\n";
    sendcontrol "* OK [UIDNEXT 4392] Predicted next UID\r\n";
    sendcontrol "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n";
    sendcontrol "* OK [PERMANENTFLAGS ()] No permanent flags permitted\r\n";
    sendcontrol "$cmdid OK [READ-ONLY] EXAMINE completed\r\n";

    return 0;
}

sub STATUS_imap {
    my ($testno) = @_;
    fix_imap_params($testno);

    logmsg "STATUS_imap got test $testno\n";

    $testno =~ s/[^0-9]//g;
    my $testpart = "";
    if ($testno > 10000) {
        $testpart = $testno % 10000;
        $testno = int($testno / 10000);
    }

    loadtest("$srcdir/data/test$testno");

    my @data = getpart("reply", "data$testpart");

    for my $d (@data) {
        sendcontrol $d;
    }

    sendcontrol "$cmdid OK STATUS completed\r\n";

    return 0;
}

sub LOGOUT_imap {
    sendcontrol "* BYE cURL IMAP server signing off\r\n";
    sendcontrol "$cmdid OK LOGOUT completed\r\n";

    return 0;
}

################
################ POP3 commands
################

sub CAPA_pop3 {
    my ($testno) = @_;
    my @data = ();

    if(!$support_capa) {
        push @data, "-ERR Unsupported command: 'CAPA'\r\n";
    }
    else {
        push @data, "+OK List of capabilities follows\r\n";
        push @data, "USER\r\n";
        if($support_auth) {
            push @data, "SASL UNKNOWN\r\n";
        }
        push @data, "IMPLEMENTATION POP3 pingpong test server\r\n";
        push @data, ".\r\n";
    }

    for my $d (@data) {
        sendcontrol $d;
    }

    return 0;
}

sub AUTH_pop3 {
    my ($testno) = @_;
    my @data = ();

    if(!$support_auth) {
        push @data, "-ERR Unsupported command: 'AUTH'\r\n";
    }
    else {
        push @data, "+OK List of supported mechanisms follows\r\n";
        push @data, "UNKNOWN\r\n";
        push @data, ".\r\n";
    }

    for my $d (@data) {
        sendcontrol $d;
    }

    return 0;
}

sub RETR_pop3 {
     my ($testno) = @_;
     my @data;

     if($testno =~ /^verifiedserver$/) {
         # this is the secret command that verifies that this actually is
         # the curl test server
         my $response = "WE ROOLZ: $$\r\n";
         if($verbose) {
             print STDERR "FTPD: We returned proof we are the test server\n";
         }
         $data[0] = $response;
         logmsg "return proof we are we\n";
     }
     else {
         logmsg "retrieve a mail\n";

         $testno =~ s/^([^0-9]*)//;
         my $testpart = "";
         if ($testno > 10000) {
             $testpart = $testno % 10000;
             $testno = int($testno / 10000);
         }

         # send mail content
         loadtest("$srcdir/data/test$testno");

         @data = getpart("reply", "data$testpart");
     }

     sendcontrol "+OK Mail transfer starts\r\n";

     for my $d (@data) {
         sendcontrol $d;
     }

     # end with the magic 3-byte end of mail marker, assumes that the
     # mail body ends with a CRLF!
     sendcontrol ".\r\n";

     return 0;
}

sub LIST_pop3 {

# this is a built-in fake-message list
my @pop3list=(
"1 100\r\n",
"2 4294967400\r\n",	# > 4 GB
"4 200\r\n", # Note that message 3 is a simulated "deleted" message
);

     logmsg "retrieve a message list\n";

     sendcontrol "+OK Listing starts\r\n";

     for my $d (@pop3list) {
         sendcontrol $d;
     }

     # end with the magic 3-byte end of listing marker
     sendcontrol ".\r\n";

     return 0;
}

################
################ FTP commands
################
my $rest=0;
sub REST_ftp {
    $rest = $_[0];
    logmsg "Set REST position to $rest\n"
}

sub switch_directory_goto {
  my $target_dir = $_;

  if(!$ftptargetdir) {
    $ftptargetdir = "/";
  }

  if($target_dir eq "") {
    $ftptargetdir = "/";
  }
  elsif($target_dir eq "..") {
    if($ftptargetdir eq "/") {
      $ftptargetdir = "/";
    }
    else {
      $ftptargetdir =~ s/[[:alnum:]]+\/$//;
    }
  }
  else {
    $ftptargetdir .= $target_dir . "/";
  }
}

sub switch_directory {
    my $target_dir = $_[0];

    if($target_dir eq "/") {
        $ftptargetdir = "/";
    }
    else {
        my @dirs = split("/", $target_dir);
        for(@dirs) {
          switch_directory_goto($_);
        }
    }
}

sub CWD_ftp {
  my ($folder, $fullcommand) = $_[0];
  switch_directory($folder);
  if($ftptargetdir =~ /^\/fully_simulated/) {
    $ftplistparserstate = "enabled";
  }
  else {
    undef $ftplistparserstate;
  }
}

sub PWD_ftp {
    my $mydir;
    $mydir = $ftptargetdir ? $ftptargetdir : "/";

    if($mydir ne "/") {
        $mydir =~ s/\/$//;
    }
    sendcontrol "257 \"$mydir\" is current directory\r\n";
}

sub LIST_ftp {
  #  print "150 ASCII data connection for /bin/ls (193.15.23.1,59196) (0 bytes)\r\n";

# this is a built-in fake-dir ;-)
my @ftpdir=("total 20\r\n",
"drwxr-xr-x   8 98       98           512 Oct 22 13:06 .\r\n",
"drwxr-xr-x   8 98       98           512 Oct 22 13:06 ..\r\n",
"drwxr-xr-x   2 98       98           512 May  2  1996 .NeXT\r\n",
"-r--r--r--   1 0        1             35 Jul 16  1996 README\r\n",
"lrwxrwxrwx   1 0        1              7 Dec  9  1999 bin -> usr/bin\r\n",
"dr-xr-xr-x   2 0        1            512 Oct  1  1997 dev\r\n",
"drwxrwxrwx   2 98       98           512 May 29 16:04 download.html\r\n",
"dr-xr-xr-x   2 0        1            512 Nov 30  1995 etc\r\n",
"drwxrwxrwx   2 98       1            512 Oct 30 14:33 pub\r\n",
"dr-xr-xr-x   5 0        1            512 Oct  1  1997 usr\r\n");

    if($datasockf_conn eq 'no') {
        if($nodataconn425) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "425 Can't open data connection\r\n";
        }
        elsif($nodataconn421) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "421 Connection timed out\r\n";
        }
        elsif($nodataconn150) {
            sendcontrol "150 Opening data connection\r\n";
            # client shall timeout
        }
        else {
            # client shall timeout
        }
        return 0;
    }

    if($ftplistparserstate) {
      @ftpdir = ftp_contentlist($ftptargetdir);
    }

    logmsg "pass LIST data on data connection\n";
    for(@ftpdir) {
        senddata $_;
    }
    close_dataconn(0);
    sendcontrol "226 ASCII transfer complete\r\n";
    return 0;
}

sub NLST_ftp {
    my @ftpdir=("file", "with space", "fake", "..", " ..", "funny", "README");

    if($datasockf_conn eq 'no') {
        if($nodataconn425) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "425 Can't open data connection\r\n";
        }
        elsif($nodataconn421) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "421 Connection timed out\r\n";
        }
        elsif($nodataconn150) {
            sendcontrol "150 Opening data connection\r\n";
            # client shall timeout
        }
        else {
            # client shall timeout
        }
        return 0;
    }

    logmsg "pass NLST data on data connection\n";
    for(@ftpdir) {
        senddata "$_\r\n";
    }
    close_dataconn(0);
    sendcontrol "226 ASCII transfer complete\r\n";
    return 0;
}

sub MDTM_ftp {
    my $testno = $_[0];
    my $testpart = "";
    if ($testno > 10000) {
        $testpart = $testno % 10000;
        $testno = int($testno / 10000);
    }

    loadtest("$srcdir/data/test$testno");

    my @data = getpart("reply", "mdtm");

    my $reply = $data[0];
    chomp $reply if($reply);

    if($reply && ($reply =~ /^[+-]?\d+$/) && ($reply < 0)) {
        sendcontrol "550 $testno: no such file.\r\n";
    }
    elsif($reply) {
        sendcontrol "$reply\r\n";
    }
    else {
        sendcontrol "500 MDTM: no such command.\r\n";
    }
    return 0;
}

sub SIZE_ftp {
    my $testno = $_[0];
    if($ftplistparserstate) {
        my $size = wildcard_filesize($ftptargetdir, $testno);
        if($size == -1) {
            sendcontrol "550 $testno: No such file or directory.\r\n";
        }
        else {
            sendcontrol "213 $size\r\n";
        }
        return 0;
    }

    if($testno =~ /^verifiedserver$/) {
        my $response = "WE ROOLZ: $$\r\n";
        my $size = length($response);
        sendcontrol "213 $size\r\n";
        return 0;
    }

    if($testno =~ /(\d+)\/?$/) {
        $testno = $1;
    }
    else {
        print STDERR "SIZE_ftp: invalid test number: $testno\n";
        return 1;
    }

    my $testpart = "";
    if($testno > 10000) {
        $testpart = $testno % 10000;
        $testno = int($testno / 10000);
    }

    loadtest("$srcdir/data/test$testno");

    my @data = getpart("reply", "size");

    my $size = $data[0];

    if($size) {
        if($size > -1) {
            sendcontrol "213 $size\r\n";
        }
        else {
            sendcontrol "550 $testno: No such file or directory.\r\n";
        }
    }
    else {
        $size=0;
        @data = getpart("reply", "data$testpart");
        for(@data) {
            $size += length($_);
        }
        if($size) {
            sendcontrol "213 $size\r\n";
        }
        else {
            sendcontrol "550 $testno: No such file or directory.\r\n";
        }
    }
    return 0;
}

sub RETR_ftp {
    my ($testno) = @_;

    if($datasockf_conn eq 'no') {
        if($nodataconn425) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "425 Can't open data connection\r\n";
        }
        elsif($nodataconn421) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "421 Connection timed out\r\n";
        }
        elsif($nodataconn150) {
            sendcontrol "150 Opening data connection\r\n";
            # client shall timeout
        }
        else {
            # client shall timeout
        }
        return 0;
    }

    if($ftplistparserstate) {
        my @content = wildcard_getfile($ftptargetdir, $testno);
        if($content[0] == -1) {
            #file not found
        }
        else {
            my $size = length $content[1];
            sendcontrol "150 Binary data connection for $testno ($size bytes).\r\n",
            senddata $content[1];
            close_dataconn(0);
            sendcontrol "226 File transfer complete\r\n";
        }
        return 0;
    }

    if($testno =~ /^verifiedserver$/) {
        # this is the secret command that verifies that this actually is
        # the curl test server
        my $response = "WE ROOLZ: $$\r\n";
        my $len = length($response);
        sendcontrol "150 Binary junk ($len bytes).\r\n";
        senddata "WE ROOLZ: $$\r\n";
        close_dataconn(0);
        sendcontrol "226 File transfer complete\r\n";
        if($verbose) {
            print STDERR "FTPD: We returned proof we are the test server\n";
        }
        return 0;
    }

    $testno =~ s/^([^0-9]*)//;
    my $testpart = "";
    if ($testno > 10000) {
        $testpart = $testno % 10000;
        $testno = int($testno / 10000);
    }

    loadtest("$srcdir/data/test$testno");

    my @data = getpart("reply", "data$testpart");

    my $size=0;
    for(@data) {
        $size += length($_);
    }

    my %hash = getpartattr("reply", "data$testpart");

    if($size || $hash{'sendzero'}) {

        if($rest) {
            # move read pointer forward
            $size -= $rest;
            logmsg "REST $rest was removed from size, makes $size left\n";
            $rest = 0; # reset REST offset again
        }
        if($retrweirdo) {
            sendcontrol "150 Binary data connection for $testno () ($size bytes).\r\n",
            "226 File transfer complete\r\n";

            for(@data) {
                my $send = $_;
                senddata $send;
            }
            close_dataconn(0);
            $retrweirdo=0; # switch off the weirdo again!
        }
        else {
            my $sz = "($size bytes)";
            if($retrnosize) {
                $sz = "size?";
            }

            sendcontrol "150 Binary data connection for $testno () $sz.\r\n";

            for(@data) {
                my $send = $_;
                senddata $send;
            }
            close_dataconn(0);
            sendcontrol "226 File transfer complete\r\n";
        }
    }
    else {
        sendcontrol "550 $testno: No such file or directory.\r\n";
    }
    return 0;
}

sub STOR_ftp {
    my $testno=$_[0];

    my $filename = "log/upload.$testno";

    if($datasockf_conn eq 'no') {
        if($nodataconn425) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "425 Can't open data connection\r\n";
        }
        elsif($nodataconn421) {
            sendcontrol "150 Opening data connection\r\n";
            sendcontrol "421 Connection timed out\r\n";
        }
        elsif($nodataconn150) {
            sendcontrol "150 Opening data connection\r\n";
            # client shall timeout
        }
        else {
            # client shall timeout
        }
        return 0;
    }

    logmsg "STOR test number $testno in $filename\n";

    sendcontrol "125 Gimme gimme gimme!\r\n";

    open(FILE, ">$filename") ||
        return 0; # failed to open output

    my $line;
    my $ulsize=0;
    my $disc=0;
    while (5 == (sysread DREAD, $line, 5)) {
        if($line eq "DATA\n") {
            my $i;
            sysread DREAD, $i, 5;

            my $size = 0;
            if($i =~ /^([0-9a-fA-F]{4})\n/) {
                $size = hex($1);
            }

            read_datasockf(\$line, $size);

            #print STDERR "  GOT: $size bytes\n";

            $ulsize += $size;
            print FILE $line if(!$nosave);
            logmsg "> Appending $size bytes to file\n";
        }
        elsif($line eq "DISC\n") {
            # disconnect!
            $disc=1;
            last;
        }
        else {
            logmsg "No support for: $line";
            last;
        }
    }
    if($nosave) {
        print FILE "$ulsize bytes would've been stored here\n";
    }
    close(FILE);
    close_dataconn($disc);
    logmsg "received $ulsize bytes upload\n";
    sendcontrol "226 File transfer complete\r\n";
    return 0;
}

sub PASV_ftp {
    my ($arg, $cmd)=@_;
    my $pasvport;
    my $bindonly = ($nodataconn) ? '--bindonly' : '';

    # kill previous data connection sockfilt when alive
    if($datasockf_runs eq 'yes') {
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt for $datasockf_mode data channel killed\n";
    }
    datasockf_state('STOPPED');

    logmsg "====> Passive DATA channel requested by client\n";

    logmsg "DATA sockfilt for passive data channel starting...\n";

    # We fire up a new sockfilt to do the data transfer for us.
    my $datasockfcmd = "./server/sockfilt " .
        "--ipv$ipvnum $bindonly --port 0 " .
        "--pidfile \"$datasockf_pidfile\" " .
        "--logfile \"$datasockf_logfile\"";
    $slavepid = open2(\*DREAD, \*DWRITE, $datasockfcmd);

    if($nodataconn) {
        datasockf_state('PASSIVE_NODATACONN');
    }
    else {
        datasockf_state('PASSIVE');
    }

    print STDERR "$datasockfcmd\n" if($verbose);

    print DWRITE "PING\n";
    my $pong;
    sysread_or_die(\*DREAD, \$pong, 5);

    if($pong =~ /^FAIL/) {
        logmsg "DATA sockfilt said: FAIL\n";
        logmsg "DATA sockfilt for passive data channel failed\n";
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        sendcontrol "500 no free ports!\r\n";
        return;
    }
    elsif($pong !~ /^PONG/) {
        logmsg "DATA sockfilt unexpected response: $pong\n";
        logmsg "DATA sockfilt for passive data channel failed\n";
        logmsg "DATA sockfilt killed now\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        sendcontrol "500 no free ports!\r\n";
        return;
    }

    logmsg "DATA sockfilt for passive data channel started (pid $slavepid)\n";

    # Find out on what port we listen on or have bound
    my $i;
    print DWRITE "PORT\n";

    # READ the response code
    sysread_or_die(\*DREAD, \$i, 5);

    # READ the response size
    sysread_or_die(\*DREAD, \$i, 5);

    my $size = 0;
    if($i =~ /^([0-9a-fA-F]{4})\n/) {
        $size = hex($1);
    }

    # READ the response data
    read_datasockf(\$i, $size);

    # The data is in the format
    # IPvX/NNN

    if($i =~ /IPv(\d)\/(\d+)/) {
        # FIX: deal with IP protocol version
        $pasvport = $2;
    }

    if(!$pasvport) {
        logmsg "DATA sockfilt unknown listener port\n";
        logmsg "DATA sockfilt for passive data channel failed\n";
        logmsg "DATA sockfilt killed now\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        sendcontrol "500 no free ports!\r\n";
        return;
    }

    if($nodataconn) {
        my $str = nodataconn_str();
        logmsg "DATA sockfilt for passive data channel ($str) bound on port ".
               "$pasvport\n";
    }
    else {
        logmsg "DATA sockfilt for passive data channel listens on port ".
               "$pasvport\n";
    }

    if($cmd ne "EPSV") {
        # PASV reply
        my $p=$listenaddr;
        $p =~ s/\./,/g;
        if($pasvbadip) {
            $p="1,2,3,4";
        }
        sendcontrol sprintf("227 Entering Passive Mode ($p,%d,%d)\n",
                            int($pasvport/256), int($pasvport%256));
    }
    else {
        # EPSV reply
        sendcontrol sprintf("229 Entering Passive Mode (|||%d|)\n", $pasvport);
    }

    logmsg "Client has been notified that DATA conn ".
           "will be accepted on port $pasvport\n";

    if($nodataconn) {
        my $str = nodataconn_str();
        logmsg "====> Client fooled ($str)\n";
        return;
    }

    eval {
        local $SIG{ALRM} = sub { die "alarm\n" };

        # assume swift operations unless explicitly slow
        alarm ($datadelay?20:10);

        # Wait for 'CNCT'
        my $input;

        # FIX: Monitor ctrl conn for disconnect

        while(sysread(DREAD, $input, 5)) {

            if($input !~ /^CNCT/) {
                # we wait for a connected client
                logmsg "Odd, we got $input from client\n";
                next;
            }
            logmsg "Client connects to port $pasvport\n";
            last;
        }
        alarm 0;
    };
    if ($@) {
        # timed out
        logmsg "$srvrname server timed out awaiting data connection ".
            "on port $pasvport\n";
        logmsg "accept failed or connection not even attempted\n";
        logmsg "DATA sockfilt killed now\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        return;
    }
    else {
        logmsg "====> Client established passive DATA connection ".
               "on port $pasvport\n";
    }

    return;
}

#
# Support both PORT and EPRT here.
#

sub PORT_ftp {
    my ($arg, $cmd) = @_;
    my $port;
    my $addr;

    # kill previous data connection sockfilt when alive
    if($datasockf_runs eq 'yes') {
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt for $datasockf_mode data channel killed\n";
    }
    datasockf_state('STOPPED');

    logmsg "====> Active DATA channel requested by client\n";

    # We always ignore the given IP and use localhost.

    if($cmd eq "PORT") {
        if($arg !~ /(\d+),(\d+),(\d+),(\d+),(\d+),(\d+)/) {
            logmsg "DATA sockfilt for active data channel not started ".
                   "(bad PORT-line: $arg)\n";
            sendcontrol "500 silly you, go away\r\n";
            return;
        }
        $port = ($5<<8)+$6;
        $addr = "$1.$2.$3.$4";
    }
    # EPRT |2|::1|49706|
    elsif($cmd eq "EPRT") {
        if($arg !~ /(\d+)\|([^\|]+)\|(\d+)/) {
            logmsg "DATA sockfilt for active data channel not started ".
                   "(bad EPRT-line: $arg)\n";
            sendcontrol "500 silly you, go away\r\n";
            return;
        }
        sendcontrol "200 Thanks for dropping by. We contact you later\r\n";
        $port = $3;
        $addr = $2;
    }
    else {
        logmsg "DATA sockfilt for active data channel not started ".
               "(invalid command: $cmd)\n";
        sendcontrol "500 we don't like $cmd now\r\n";
        return;
    }

    if(!$port || $port > 65535) {
        logmsg "DATA sockfilt for active data channel not started ".
               "(illegal PORT number: $port)\n";
        return;
    }

    if($nodataconn) {
        my $str = nodataconn_str();
        logmsg "DATA sockfilt for active data channel not started ($str)\n";
        datasockf_state('ACTIVE_NODATACONN');
        logmsg "====> Active DATA channel not established\n";
        return;
    }

    logmsg "DATA sockfilt for active data channel starting...\n";

    # We fire up a new sockfilt to do the data transfer for us.
    my $datasockfcmd = "./server/sockfilt " .
        "--ipv$ipvnum --connect $port --addr \"$addr\" " .
        "--pidfile \"$datasockf_pidfile\" " .
        "--logfile \"$datasockf_logfile\"";
    $slavepid = open2(\*DREAD, \*DWRITE, $datasockfcmd);

    datasockf_state('ACTIVE');

    print STDERR "$datasockfcmd\n" if($verbose);

    print DWRITE "PING\n";
    my $pong;
    sysread_or_die(\*DREAD, \$pong, 5);

    if($pong =~ /^FAIL/) {
        logmsg "DATA sockfilt said: FAIL\n";
        logmsg "DATA sockfilt for active data channel failed\n";
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        # client shall timeout awaiting connection from server
        return;
    }
    elsif($pong !~ /^PONG/) {
        logmsg "DATA sockfilt unexpected response: $pong\n";
        logmsg "DATA sockfilt for active data channel failed\n";
        logmsg "DATA sockfilt killed now\n";
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt not running\n";
        datasockf_state('STOPPED');
        # client shall timeout awaiting connection from server
        return;
    }

    logmsg "DATA sockfilt for active data channel started (pid $slavepid)\n";

    logmsg "====> Active DATA channel connected to client port $port\n";

    return;
}

#**********************************************************************
# datasockf_state is used to change variables that keep state info
# relative to the FTP secondary or data sockfilt process as soon as
# one of the five possible stable states is reached. Variables that
# are modified by this sub may be checked independently but should
# not be changed except by calling this sub.
#
sub datasockf_state {
    my $state = $_[0];

  if($state eq 'STOPPED') {
    # Data sockfilter initial state, not running,
    # not connected and not used.
    $datasockf_state = $state;
    $datasockf_mode = 'none';
    $datasockf_runs = 'no';
    $datasockf_conn = 'no';
  }
  elsif($state eq 'PASSIVE') {
    # Data sockfilter accepted connection from client.
    $datasockf_state = $state;
    $datasockf_mode = 'passive';
    $datasockf_runs = 'yes';
    $datasockf_conn = 'yes';
  }
  elsif($state eq 'ACTIVE') {
    # Data sockfilter has connected to client.
    $datasockf_state = $state;
    $datasockf_mode = 'active';
    $datasockf_runs = 'yes';
    $datasockf_conn = 'yes';
  }
  elsif($state eq 'PASSIVE_NODATACONN') {
    # Data sockfilter bound port without listening,
    # client won't be able to establish data connection.
    $datasockf_state = $state;
    $datasockf_mode = 'passive';
    $datasockf_runs = 'yes';
    $datasockf_conn = 'no';
  }
  elsif($state eq 'ACTIVE_NODATACONN') {
    # Data sockfilter does not even run,
    # client awaits data connection from server in vain.
    $datasockf_state = $state;
    $datasockf_mode = 'active';
    $datasockf_runs = 'no';
    $datasockf_conn = 'no';
  }
  else {
      die "Internal error. Unknown datasockf state: $state!";
  }
}

#**********************************************************************
# nodataconn_str returns string of efective nodataconn command. Notice
# that $nodataconn may be set alone or in addition to a $nodataconnXXX.
#
sub nodataconn_str {
    my $str;
    # order matters
    $str = 'NODATACONN' if($nodataconn);
    $str = 'NODATACONN425' if($nodataconn425);
    $str = 'NODATACONN421' if($nodataconn421);
    $str = 'NODATACONN150' if($nodataconn150);
    return "$str";
}

#**********************************************************************
# customize configures test server operation for each curl test, reading
# configuration commands/parameters from server commands file each time
# a new client control connection is established with the test server.
# On success returns 1, otherwise zero.
#
sub customize {
    $ctrldelay = 0;     # default is no throttling of the ctrl stream
    $datadelay = 0;     # default is no throttling of the data stream
    $retrweirdo = 0;    # default is no use of RETRWEIRDO
    $retrnosize = 0;    # default is no use of RETRNOSIZE
    $pasvbadip = 0;     # default is no use of PASVBADIP
    $nosave = 0;        # default is to actually save uploaded data to file
    $nodataconn = 0;    # default is to establish or accept data channel
    $nodataconn425 = 0; # default is to not send 425 without data channel
    $nodataconn421 = 0; # default is to not send 421 without data channel
    $nodataconn150 = 0; # default is to not send 150 without data channel
    $support_capa = 0;  # default is to not support capability command
    $support_auth = 0;  # default is to not support authentication command
    %customreply = ();  #
    %customcount = ();  #
    %delayreply = ();   #

    open(CUSTOM, "<log/ftpserver.cmd") ||
        return 1;

    logmsg "FTPD: Getting commands from log/ftpserver.cmd\n";

    while(<CUSTOM>) {
        if($_ =~ /REPLY ([A-Za-z0-9+\/=]+) (.*)/) {
            $customreply{$1}=eval "qq{$2}";
            logmsg "FTPD: set custom reply for $1\n";
        }
        elsif($_ =~ /COUNT ([A-Z]+) (.*)/) {
            # we blank the customreply for this command when having
            # been used this number of times
            $customcount{$1}=$2;
            logmsg "FTPD: blank custom reply for $1 after $2 uses\n";
        }
        elsif($_ =~ /DELAY ([A-Z]+) (\d*)/) {
            $delayreply{$1}=$2;
            logmsg "FTPD: delay reply for $1 with $2 seconds\n";
        }
        elsif($_ =~ /SLOWDOWN/) {
            $ctrldelay=1;
            $datadelay=1;
            logmsg "FTPD: send response with 0.01 sec delay between each byte\n";
        }
        elsif($_ =~ /RETRWEIRDO/) {
            logmsg "FTPD: instructed to use RETRWEIRDO\n";
            $retrweirdo=1;
        }
        elsif($_ =~ /RETRNOSIZE/) {
            logmsg "FTPD: instructed to use RETRNOSIZE\n";
            $retrnosize=1;
        }
        elsif($_ =~ /PASVBADIP/) {
            logmsg "FTPD: instructed to use PASVBADIP\n";
            $pasvbadip=1;
        }
        elsif($_ =~ /NODATACONN425/) {
            # applies to both active and passive FTP modes
            logmsg "FTPD: instructed to use NODATACONN425\n";
            $nodataconn425=1;
            $nodataconn=1;
        }
        elsif($_ =~ /NODATACONN421/) {
            # applies to both active and passive FTP modes
            logmsg "FTPD: instructed to use NODATACONN421\n";
            $nodataconn421=1;
            $nodataconn=1;
        }
        elsif($_ =~ /NODATACONN150/) {
            # applies to both active and passive FTP modes
            logmsg "FTPD: instructed to use NODATACONN150\n";
            $nodataconn150=1;
            $nodataconn=1;
        }
        elsif($_ =~ /NODATACONN/) {
            # applies to both active and passive FTP modes
            logmsg "FTPD: instructed to use NODATACONN\n";
            $nodataconn=1;
        }
        elsif($_ =~ /SUPPORTCAPA/) {
            logmsg "FTPD: instructed to support CAPABILITY command\n";
            $support_capa=1;
        }
        elsif($_ =~ /SUPPORTAUTH/) {
            logmsg "FTPD: instructed to support AUTHENTICATION command\n";
            $support_auth=1;
        }
        elsif($_ =~ /NOSAVE/) {
            # don't actually store the file we upload - to be used when
            # uploading insanely huge amounts
            $nosave = 1;
            logmsg "FTPD: NOSAVE prevents saving of uploaded data\n";
        }
    }
    close(CUSTOM);
}

#----------------------------------------------------------------------
#----------------------------------------------------------------------
#---------------------------  END OF SUBS  ----------------------------
#----------------------------------------------------------------------
#----------------------------------------------------------------------

#**********************************************************************
# Parse command line options
#
# Options:
#
# --verbose   # verbose
# --srcdir    # source directory
# --id        # server instance number
# --proto     # server protocol
# --pidfile   # server pid file
# --logfile   # server log file
# --ipv4      # server IP version 4
# --ipv6      # server IP version 6
# --port      # server listener port
# --addr      # server address for listener port binding
#
while(@ARGV) {
    if($ARGV[0] eq '--verbose') {
        $verbose = 1;
    }
    elsif($ARGV[0] eq '--srcdir') {
        if($ARGV[1]) {
            $srcdir = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--id') {
        if($ARGV[1] && ($ARGV[1] =~ /^(\d+)$/)) {
            $idnum = $1 if($1 > 0);
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--proto') {
        if($ARGV[1] && ($ARGV[1] =~ /^(ftp|imap|pop3|smtp)$/)) {
            $proto = $1;
            shift @ARGV;
        }
        else {
            die "unsupported protocol $ARGV[1]";
        }
    }
    elsif($ARGV[0] eq '--pidfile') {
        if($ARGV[1]) {
            $pidfile = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--logfile') {
        if($ARGV[1]) {
            $logfile = $ARGV[1];
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--ipv4') {
        $ipvnum = 4;
        $listenaddr = '127.0.0.1' if($listenaddr eq '::1');
    }
    elsif($ARGV[0] eq '--ipv6') {
        $ipvnum = 6;
        $listenaddr = '::1' if($listenaddr eq '127.0.0.1');
    }
    elsif($ARGV[0] eq '--port') {
        if($ARGV[1] && ($ARGV[1] =~ /^(\d+)$/)) {
            $port = $1 if($1 > 1024);
            shift @ARGV;
        }
    }
    elsif($ARGV[0] eq '--addr') {
        if($ARGV[1]) {
            my $tmpstr = $ARGV[1];
            if($tmpstr =~ /^(\d\d?\d?)\.(\d\d?\d?)\.(\d\d?\d?)\.(\d\d?\d?)$/) {
                $listenaddr = "$1.$2.$3.$4" if($ipvnum == 4);
            }
            elsif($ipvnum == 6) {
                $listenaddr = $tmpstr;
                $listenaddr =~ s/^\[(.*)\]$/$1/;
            }
            shift @ARGV;
        }
    }
    else {
        print STDERR "\nWarning: ftpserver.pl unknown parameter: $ARGV[0]\n";
    }
    shift @ARGV;
}

#***************************************************************************
# Initialize command line option dependant variables
#

if(!$srcdir) {
    $srcdir = $ENV{'srcdir'} || '.';
}
if(!$pidfile) {
    $pidfile = "$path/". server_pidfilename($proto, $ipvnum, $idnum);
}
if(!$logfile) {
    $logfile = server_logfilename($logdir, $proto, $ipvnum, $idnum);
}

$mainsockf_pidfile = "$path/".
    mainsockf_pidfilename($proto, $ipvnum, $idnum);
$mainsockf_logfile =
    mainsockf_logfilename($logdir, $proto, $ipvnum, $idnum);

if($proto eq 'ftp') {
    $datasockf_pidfile = "$path/".
        datasockf_pidfilename($proto, $ipvnum, $idnum);
    $datasockf_logfile =
        datasockf_logfilename($logdir, $proto, $ipvnum, $idnum);
}

$srvrname = servername_str($proto, $ipvnum, $idnum);

$idstr = "$idnum" if($idnum > 1);

protocolsetup($proto);

$SIG{INT} = \&exit_signal_handler;
$SIG{TERM} = \&exit_signal_handler;

startsf();

logmsg sprintf("%s server listens on port IPv${ipvnum}/${port}\n", uc($proto));

open(PID, ">$pidfile");
print PID $$."\n";
close(PID);

logmsg("logged pid $$ in $pidfile\n");


while(1) {

    # kill previous data connection sockfilt when alive
    if($datasockf_runs eq 'yes') {
        killsockfilters($proto, $ipvnum, $idnum, $verbose, 'data');
        logmsg "DATA sockfilt for $datasockf_mode data channel killed now\n";
    }
    datasockf_state('STOPPED');

    #
    # We read 'sockfilt' commands.
    #
    my $input;

    logmsg "Awaiting input\n";
    sysread_or_die(\*SFREAD, \$input, 5);

    if($input !~ /^CNCT/) {
        # we wait for a connected client
        logmsg "MAIN sockfilt said: $input";
        next;
    }
    logmsg "====> Client connect\n";

    set_advisor_read_lock($SERVERLOGS_LOCK);
    $serverlogslocked = 1;

    # flush data:
    $| = 1;

    &customize(); # read test control instructions

    my $welcome = $customreply{"welcome"};
    if(!$welcome) {
        $welcome = $displaytext{"welcome"};
    }
    else {
        # clear it after use
        $customreply{"welcome"}="";
        if($welcome !~ /\r\n\z/) {
            $welcome .= "\r\n";
        }
    }
    sendcontrol $welcome;

    #remove global variables from last connection
    if($ftplistparserstate) {
      undef $ftplistparserstate;
    }
    if($ftptargetdir) {
      undef $ftptargetdir;
    }

    if($verbose) {
        print STDERR "OUT: $welcome";
    }

    my $full = "";

    while(1) {
        my $i;

        # Now we expect to read DATA\n[hex size]\n[prot], where the [prot]
        # part only is FTP lingo.

        # COMMAND
        sysread_or_die(\*SFREAD, \$i, 5);

        if($i !~ /^DATA/) {
            logmsg "MAIN sockfilt said $i";
            if($i =~ /^DISC/) {
                # disconnect
                last;
            }
            next;
        }

        # SIZE of data
        sysread_or_die(\*SFREAD, \$i, 5);

        my $size = 0;
        if($i =~ /^([0-9a-fA-F]{4})\n/) {
            $size = hex($1);
        }

        # data
        read_mainsockf(\$input, $size);

        ftpmsg $input;

        $full .= $input;

        # Loop until command completion
        next unless($full =~ /\r\n$/);

        # Remove trailing CRLF.
        $full =~ s/[\n\r]+$//;

        my $FTPCMD;
        my $FTPARG;
        if($proto eq "imap") {
            # IMAP is different with its identifier first on the command line
            unless(($full =~ /^([^ ]+) ([^ ]+) (.*)/) ||
                   ($full =~ /^([^ ]+) ([^ ]+)/)) {
                sendcontrol "$1 '$full': command not understood.\r\n";
                last;
            }
            $cmdid=$1; # set the global variable
            $FTPCMD=$2;
            $FTPARG=$3;
        }
        elsif($full =~ /^([A-Z]{3,4})(\s(.*))?$/i) {
            $FTPCMD=$1;
            $FTPARG=$3;
        }
        elsif(($proto eq "smtp") && ($full =~ /^[A-Z0-9+\/]{0,512}={0,2}$/i)) {
            # SMTP long "commands" are base64 authentication data.
            $FTPCMD=$full;
            $FTPARG="";
        }
        else {
            sendcontrol "500 '$full': command not understood.\r\n";
            last;
        }

        logmsg "< \"$full\"\n";

        if($verbose) {
            print STDERR "IN: $full\n";
        }

        $full = "";

        my $delay = $delayreply{$FTPCMD};
        if($delay) {
            # just go sleep this many seconds!
            logmsg("Sleep for $delay seconds\n");
            my $twentieths = $delay * 20;
            while($twentieths--) {
                select(undef, undef, undef, 0.05) unless($got_exit_signal);
            }
        }

        my $text;
        $text = $customreply{$FTPCMD};
        my $fake = $text;

        if($text && ($text ne "")) {
            if($customcount{$FTPCMD} && (!--$customcount{$FTPCMD})) {
                # used enough number of times, now blank the customreply
                $customreply{$FTPCMD}="";
            }
        }
        else {
            $text = $displaytext{$FTPCMD};
        }
        my $check;
        if($text && ($text ne "")) {
            if($cmdid && ($cmdid ne "")) {
                sendcontrol "$cmdid$text\r\n";
            }
            else {
                sendcontrol "$text\r\n";
            }
        }
        else {
            $check=1; # no response yet
        }

        unless($fake && ($fake ne "")) {
            # only perform this if we're not faking a reply
            my $func = $commandfunc{$FTPCMD};
            if($func) {
                &$func($FTPARG, $FTPCMD);
                $check=0; # taken care of
            }
        }

        if($check) {
            logmsg "$FTPCMD wasn't handled!\n";
            if($proto eq 'pop3') {
                sendcontrol "-ERR $FTPCMD is not dealt with!\r\n";
            }
            elsif($proto eq 'imap') {
                sendcontrol "$cmdid BAD $FTPCMD is not dealt with!\r\n";
            }
            else {
                sendcontrol "500 $FTPCMD is not dealt with!\r\n";
            }
        }

    } # while(1)
    logmsg "====> Client disconnected\n";

    if($serverlogslocked) {
        $serverlogslocked = 0;
        clear_advisor_read_lock($SERVERLOGS_LOCK);
    }
}

killsockfilters($proto, $ipvnum, $idnum, $verbose);
unlink($pidfile);
if($serverlogslocked) {
    $serverlogslocked = 0;
    clear_advisor_read_lock($SERVERLOGS_LOCK);
}

exit;
