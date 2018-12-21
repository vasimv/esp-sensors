#!/usr/bin/perl

use strict;

my $openhabItemsDir = "/etc/openhab2/items";
my $brokerName = "mybroker";
my $groupsFile = $openhabItemsDir . '/' . '000groups.items';

my %topics;

open(INTOPICS, "timeout 10 mosquitto_sub -u testuser -P testtest -t /myhome/PNP/# -v |");
while (<INTOPICS>) {
	if (/^\/myhome\/PNP(\/[^\/][^\s]+)\s+(.*)$/) {
		$topics{$1} = $2;
	}
}
close(INTOPICS);

my %devices;
my %allgroups;

foreach my $key (keys %topics) {
	print "Topic: $key\n";
	my @subkeys = split(/\//, $key);
	if (!exists($devices{$subkeys[2]})) {
		$devices{$subkeys[2]} = {};
	}
	if (!exists($devices{$subkeys[2]}{$subkeys[3]})) {
		$devices{$subkeys[2]}{$subkeys[3]} = {'topic' => '/' . $subkeys[1] . '/' . $subkeys[2] . '/' . $subkeys[3] . '/status'};
	}
	$devices{$subkeys[2]}{$subkeys[3]}{$subkeys[4]} = $topics{$key};
}

# Add new groups to a groups file (and be sure it will be reloaded before all others)
foreach my $device (keys %devices) {
	my @deviceParts = split(/[_-]/, $device);
	my %items = %{$devices{$device}};
	$allgroups{$deviceParts[0]} = 1;
	# Find all groups
	foreach my $item (keys %items) {
		my %cur = %{$items{$item}};
		my @curGroups = split(/ /, $cur{'groups'});
		push @curGroups, $deviceParts[0];
		$items{$item}{'arrGroups'} = \@curGroups;
		foreach my $addGroup (@curGroups) {
			$allgroups{$addGroup} = 1;
		}
	}
}

my %curgroups;
open(IN, $groupsFile);
while (<IN>) {
	chomp;
	if (/^\s*#?\s*Group\s+([^\s]*)\s*$/) {
		$curgroups{$1} = 1;
	}
}
close(IN);
open(OUT, ">>" . $groupsFile);
foreach my $group (keys %allgroups) {
	if (!exists($curgroups{$group}) && ("$group" ne "")) {
		print OUT "Group $group\n";
	}
}
close(OUT);
system("chown openhab:openhab $groupsFile");
sleep(5);

foreach my $device (keys %devices) {
	my $deviceFile = $openhabItemsDir . "/" . $device . ".items";
	if (! -f $deviceFile && ("$device" ne "")) {
		print "Creating items file $deviceFile\n";
		open(OUT, ">", $deviceFile);
		my %items = %{$devices{$device}};
		my @deviceParts = split(/[_-]/, $device);
		print $deviceParts[0] . "\n";
		foreach my $item (sort keys %items) {
			my %cur = %{$items{$item}};
			my $itemGroups = join(',', @{$items{$item}{'arrGroups'}});
			my @itemType = split(/:/, $cur{'type'});
			if (exists $itemType[1]) {
				my $fullName = $device . "_" . $item;
				$fullName =~ s/-/_/g;
				print OUT $itemType[1] . " " . $fullName . " \""
					. $fullName . " " . $cur{'name'} . " [%s]\"";
				print OUT " (" . $itemGroups . ") ";
				my $inTopic = ">[$brokerName:" . $cur{'topic'} . ":state:*:default]";
				my $outTopic = "<[$brokerName:" . $cur{'topic'} . ":state:default]";
				if ($itemType[0] eq 'I') {
					print OUT "{mqtt=\"$inTopic\"}";
				} elsif ($itemType[0] eq 'O') {
					print OUT "{mqtt=\"$outTopic\"}";
				} else {
					print OUT "{mqtt=\"$inTopic,$outTopic\"}";
				}
			}
			print OUT "\n";
		}
		close(OUT);
		system("chown openhab:openhab $deviceFile");
	}
}

