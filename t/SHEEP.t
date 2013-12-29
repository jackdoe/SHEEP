# Before 'make install' is performed this script should be runnable with
# 'make test'. After 'make install' it should work as 'perl SHEEP.t'

#########################

# change 'tests => 1' to 'tests => last_test_to_print';

use strict;
use warnings;
use Data::Dumper;
use Test::More;
use File::Temp;
use File::Path qw(make_path);
use File::Spec;
use utf8;
BEGIN { use_ok('SHEEP') };

my $root = File::Temp->newdir( 'ZZZ_SHEEP_test_index_ZZZ_XXXX', CLEANUP => 1 );
my $shards = 10;

my $x = new SHEEP($root,$shards);
my @documents = ();

push @documents, { "name" => "jack doe 杰克 多伊", age => "200" }
    for(1 .. 1_000);

push @documents, { "name" => "john doe ", age => "300" }
    for(1 .. 1_000);

my $n = scalar(@documents);

is ($x->index(\@documents), $n,"should be " . $n);

my $utf = $x->search({ term => { name => '杰克' } },1);
is ($utf->[0]->{name}, "jack doe 杰克 多伊");
is (scalar(@{ $x->search({ term => { name => 'doe' } },$n) }), $n);
is (scalar(@{ $x->search({ term => { name => '杰克' } },$n) }), $n / 2);
is (scalar(@{ $x->search({ term => { name => 'doe', age => '200' } },$n) }), $n / 2);
is (scalar(@{ $x->search({ term => { name => 'doe', age => '10' } },$n) }), 0);
is (scalar(@{ $x->search({ term => { name => 'doe' } },10) }), 10 * $shards);

is (scalar(@{ $x->search({ 
                            bool => {
                                must => [
                                   { term => { "name" => 'jack' } }
                                ],
                                must_not => [
                                   { term => { "name" => 'john' } }
                                ]
                            }
                         }, $n) }), $n / 2, "find only jack");


is (scalar(@{ $x->search({ 
                            bool => {
                                must => [
                                   { term => { "name" => 'jack' } }
                                ],
                                must_not => [
                                   { term => { "name" => 'jack' } }
                                ]
                            }
                         }, $n) }), 0, "find jack without jack");

is (scalar(@{ $x->search({ 
                            bool => {
                                must => [
                                   { term => { "name" => 'jack' } }
                                ],
                                should => [
                                   { term => { "name" => 'jim' } }
                                ],
                                must_not => [
                                   { term => { "name" => 'john' } }
                                ]
                            }
                         }, $n) }), $n / 2, "jack and maybe jim, without john");



is (scalar(@{ $x->search({ 
                            bool => {
                                must => [
                                   { 
                                       bool => {  
                                           must => [ { term => { name => 'jack' } } ]
                                       }
                                   }
                                ],
                                should => [
                                   { term => { "name" => 'jim' } }
                                ],
                                must_not => [
                                   { term => { "name" => 'john' } }
                                ]
                            }
                         }, $n) }), $n / 2, "double bool - jack and maybe jim, without john");

# add $n more documents
is ($x->index(\@documents), $n,"should be " . $n);
is (scalar(@{ $x->search({ term => { name => 'doe' } },$n) }), $n * 2, "after adding $n more documents");
is (scalar(@{ $x->search({ term => { name => 'doe' } },10) }), 10 * $shards);

done_testing;
#########################

# Insert your test code below, the Test::More module is use()ed here so read
# its man page ( perldoc Test::More ) for help writing this test script.

