#!/usr/bin/perl

use strict;
use warnings;

my $path_info = $ENV{'PATH_INFO'};
print "PATH_INFO = $path_info\n";


if ($path_info eq '/env') {
    # Print environment variables
    print "Content-Type: text/html\n\n";
    print "<html><body>";
    print "<h1>Environment Variables:</h1>";
    print "<pre>";

    foreach my $key (keys %ENV) {
        print "$key: $ENV{$key}\n";
    }

    print "</pre>";
    print "</body></html>";
} else {
    # Print a simple message
    print "Content-Type: text/html\n\n";
    print "<html><body>";
    print "<h1>Hello from Perl!</h1>";
    print "</body></html>";
}