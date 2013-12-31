use Benchmark;
use strict;
use warnings;
use Data::Dumper;
use File::Temp;
use SHEEP;
my $root = File::Temp->newdir( 'ZZZ_SHEEP_bench_index_ZZZ_XXXX', CLEANUP => 1 );
my $shards = 10;

my $x = new SHEEP($root,$shards);
my @documents = ();
for my $n((100_000,1_000_000)) {
    push @documents, { "name" => "jack doe_$n" }
        for(1 .. $n);
}
$x->index(\@documents);
@documents = ();

timethese(1_000, {
    '100_000' => sub {
        my $r = $x->search({ term => { name => 'doe_100000' } },1);
    },
    '1_000_000' => sub {
        my $r = $x->search({ term => { name => 'doe_1000000' } },1);
    }
});

undef $x;
