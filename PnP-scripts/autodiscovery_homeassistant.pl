#!/usr/bin/perl

use strict;

my $hassItemsDir = "/home/homeassistant/.homeassistant";
my $brokerName = "mybroker";
my $hassIgnoreDir = "/home/homeassistant/.homeassistant/ignore";

my %topics;


sub genTextSensor;
sub genTextDimmer;
sub genTextSwitch;

open(INTOPICS, "timeout 10 mosquitto_sub -u testuser -P testtest -t /myhome/PNP/# -v |");
while (<INTOPICS>) {
	if (/^\/myhome\/PNP(\/[^\/][^\s]+)\s+(.*)$/) {
		$topics{$1} = $2;
	}
}
close(INTOPICS);

my %devices;

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
use Data::Dumper;
print Dumper \%devices;

my %groups;
my @excludeHistory;

foreach my $device (keys %devices) {
	my %items = %{$devices{$device}};
	my @deviceParts = split(/[_-]/, $device);
	print "Device $device\n";
	foreach my $item (keys %items) {
		print "Item $item\n";
		my %cur = %{$items{$item}};
		my $fullId = $device . "_" . $item;
		$fullId =~ s/-//g;
		my $name = $fullId . ' ' . $cur{'name'};
		$cur{'descname'} = $name;
		my @curGroups = split(/ /, $cur{'groups'});
		my @itemType = split(/:/, $cur{'type'});
		my %existGroups = map { $_ => 1 } @curGroups;
		my $hassItemDom;
		my $hassItemText;
		if ($itemType[1] eq 'Number') {
			$hassItemDom = 'sensor';
			$hassItemText = genTextSensor(\%cur);
		} elsif ($itemType[1] eq 'Dimmer') {
			$hassItemDom = 'light';
			$hassItemText = genTextDimmer(\%cur);
		} else {
			$hassItemDom = 'switch';
			$hassItemText = genTextSwitch(\%cur);
		}
		# Disable history for movement sensors, buttons, builtin leds and such
		if (exists($existGroups{'Movement'}) || exists($existGroups{'Builtin'}) || exists($existGroups{'ADC'})) {
			my $sensId = lc($name);
			$sensId =~ s/ /_/g;
			$sensId =~ s/-//g;
			push @excludeHistory, $hassItemDom . '.' . $sensId;
		}
		my $itemFile = $hassItemsDir . '/' . $hassItemDom . '/' . $fullId . '.yaml';
		if (! -f $itemFile) {
			open(OUT, ">" . $itemFile);
			print OUT $hassItemText;
			close(OUT);
			system("chown homeassistant:homeassistant $itemFile");
		}
	}
}

# Add ignore files (you can comment inside the file later!)
foreach my $ignore (@excludeHistory) {
	my $ignoreFile = $hassIgnoreDir . "/" . $ignore . ".yaml";
	if (! -f $ignoreFile) {
		open(OUT, ">" . $ignoreFile);
		print OUT "$ignore\n";
		close(OUT);
	}
}
exit(0);

sub genTextSensor {
	my $item = shift;
	
	my $text = <<END_TEXT;
platform: mqtt
state_topic: "$item->{'topic'}"
name: "$item->{'descname'}"
END_TEXT
	return $text;
}

sub genTextDimmer {
	my $item = shift;
	
	my $text = <<END_TEXT;
platform: mqtt_template
command_topic: "$item->{'topic'}"
name: "$item->{'descname'}"
qos: 1
command_on_template: >
  {\% if brightness is defined \%}
    {{ (brightness * 100 / 255)| round(0) }}
  {\% else \%}
    100
  {\% endif \%}
command_off_template: "{{ 0 }}"
brightness_template: >
  {\% if value == 'OFF' or value == 'off' \%}
    0
  {\% else \%}
    {\% if value == 'ON' or value == 'on' \%}
      255
    {\% else \%}
      {{ (float(value) * 255 / 100) | round(0) }}
    {\% endif \%}
  {\% endif \%}
optimistic: true
state_template: >
  {\% if (value | float > 0.0) \%}
    on
  {\% else \%}
    off
  {\% endif \%}
END_TEXT
	return $text;
}

sub genTextSwitch {
	my $item = shift;

	my $text = <<END_TEXT;
platform: mqtt
name: "$item->{'descname'}"
state_topic: "$item->{'topic'}"
command_topic: "$item->{'topic'}"
payload_on: "ON"
payload_off: "OFF"
optimistic: false
qos: 1
retain: true
END_TEXT
	return $text;
}
