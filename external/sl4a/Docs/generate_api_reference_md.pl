#!/usr/bin/perl
#
#  Copyright (C) 2015 Google, Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at:
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

use strict;
use warnings;
use Cwd 'abs_path';
use JSON;
use File::Find;

my $sl4a_path = $ARGV[0];
my $md = "";
my $md_end = "";

if (not defined $sl4a_path) {
    $sl4a_path = abs_path($0);
    $sl4a_path =~ s/\/Docs\/generate_api_reference_md\.pl//g;
}

sub eachFile {
    my $filename = $_;
    my $fullpath = $File::Find::name;
    if (-e $filename && $filename =~ m/Facade\.java/) {
        open(FILE, $filename);
        my @lines = <FILE>;
        close(FILE);

        my $title = $filename;
        $title =~ s/\.java//;
        $title = '**' . $title . '**' . "\n";
        $md = $md . "\n$title";
        my $description = "";
        for (my $i = 0; $i < scalar(@lines); $i++) {
            my $line = $lines[$i];
            $line =~ s/\n//;
            $line =~ s/^\s+|\s+$//g;

            if ($line =~ m /^\@Rpc\(description/) {
                $description = "";
                for (my $j = $i; $j < scalar(@lines); $j++) {
                    my $l = $lines[$j];
                    $l =~ s/^\s+|\s+$//g;
                    $description = $description . $l;
                    if ($l =~ m/\)$/) {
                        $i = $j;
                        last;
                    }
                }
                $description = _format_description($description);

            }
            if ($line =~ m /^public/ && $description ne "") {
                my @words = split(/\s/, $line);
                my $func_name = $words[2];
                my $func_names_and_params = "";
                if ($func_name =~ /void/) {
                    $func_name = $words[3];
                    if ($func_name =~ /void/) {
                        $description = "";
                        $func_names_and_params = "";
                        next;
                    }
                }
                if ($func_name =~ /\(/) {
                    $func_name =~ s/\(.*//;
                }
                $func_name =~ s/\(//g;
                $func_name =~ s/\)//g;
                for (my $j = $i; $j < scalar(@lines); $j++) {
                    $func_names_and_params = $func_names_and_params . $lines[$j];
                    if ($lines[$j] =~ m/{$/) {
                        last;
                    }
                }
                $func_names_and_params = _format_func_names_and_params($func_names_and_params);
                if ($func_names_and_params eq "") {
                    $func_names_and_params = ")\n";
                } else {
                    $func_names_and_params = "\n" . $func_names_and_params;
                }
                $md_end = $md_end . "# $func_name\n```\n" .
                    "$func_name(" . $func_names_and_params . "\n$description\n```\n\n" ;
                $description = "";
                $func_names_and_params = "";
                my $lc_name = lc $func_name;
                $md = $md . "  * [$func_name](\#$lc_name)\n";
            }
        }

    }
}

sub _format_func_names_and_params {
    my $fn = shift;
    $fn =~ s/^\s+|\s+$//g;
    my @words = split(/\n/,$fn);
    my $format = "";
    my $description = "";
    my $name = "";
    my $params = "";
    for my $w (@words) {
        if ($w =~ /\@RpcParameter\(name = "(.+?)", description = "(.+?)"/) {
           $name = $1;
           $description = $2;
        }
        elsif ($w =~ /\@RpcParameter\(name = "(.+?)"/) {
           $name = $1;
        }
        if ($w =~ m/,$/) {
            my @split = split(/\s/, $w);
            $params = "$split[$#split-1] $split[$#split]";
            if ($description eq "") {
                $format = $params;
            } elsif ($description ne "") {
                $params =~ s/,//;
                $format = $format . "  $params: $description,\n"
            }
            $description = "";
            $name = "";
            $params = "";
        }
    }
    $format =~ s/,$/)/;
    return $format;
}

sub _format_description {
    my $description = shift;
    $description =~ s/\@Rpc\(//;
    $description =~ s/^\s+|\s+$//g;
    $description =~ s/\n//g;
    $description =~ s/description = \"//g;
    $description =~ s/\"\)//g;
    if ($description =~ m/returns(\s*)=/) {
        $description =~ s/\",//;
        my @words = split(/returns(\s*)=/, $description);
        my $des = $words[0];
        my $ret = $words[1];
        $ret =~ s/^\s+|\s+$//g;
        $ret =~ s/^"//;
        $description = $des . "\n\n" . "Returns:\n" . "  $ret";
    }
    return $description;
}

find (\&eachFile, $sl4a_path);
open(FILE, ">$sl4a_path/Docs/ApiReference.md");
print FILE $md . "\n";
print FILE $md_end . "\n";
close(FILE);
