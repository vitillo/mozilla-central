#!/usr/local/bin/perl
use strict;
require "genverifier.pm";
use genverifier;


my(@euckr_cls);
my(@euckr_st);
my($euckr_ver);


@euckr_cls = (
 [ 0x00 , 0x00 , 0 ],
 [ 0x1b , 0x1b , 0 ],
 [ 0x01 , 0x7f , 1 ],
 [ 0x80 , 0xa0 , 0 ],
 [ 0xff , 0xff , 0 ],
 [ 0xad , 0xaf , 3 ],
 [ 0xc9 , 0xc9 , 3 ],
 [ 0xa1 , 0xfe , 2 ],
);

package genverifier;
@euckr_st = (
#  0  1  2  3  
   1, 0, 3, 1,  # state 0
   1, 1, 1, 1,  # Error State - 1
   2, 2, 2, 2,  # ItsMe State - 2
   1, 1, 0, 0,  # state 3
);


$euckr_ver = genverifier::GenVerifier("EUCKR", "EUC-KR", \@euckr_cls, 4,     \@euckr_st);
print $euckr_ver;



