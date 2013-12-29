package SHEEP;

use 5.018001;
use strict;
use warnings;

require Exporter;

our @ISA = qw(Exporter);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

# This allows declaration	use SHEEP ':all';
# If you do not need this, moving things directly into @EXPORT or @EXPORT_OK
# will save memory.
our %EXPORT_TAGS = ( 'all' => [ qw(
	
) ] );

our @EXPORT_OK = ( @{ $EXPORT_TAGS{'all'} } );

our @EXPORT = qw(

);

our $VERSION = '0.01';

require XSLoader;
XSLoader::load('SHEEP', $VERSION);

# Preloaded methods go here.

1;
__END__
# Below is stub documentation for your module. You'd better edit it!
=head1 ALPHA - WORK IN PROGRESS

*alpha* not recommended for anything but hacking on some simple CLucene examples

=head1 NAME

SHEEP - simple CLucene wrapper that spawns threads for every shard when search()ing

=head1 SYNOPSIS

  use SHEEP;
  my $x = new SHEEP('/path/to/index',5) # create index with 5 shards

  my @documents = ();
  push @documents, { "name" => "jack doe", age => "200" }
      for(1 .. 1_000);

  push @documents, { "name" => "john doe", age => "300" }
      for(1 .. 1_000);

  # find top 10 documents containing 'doe' in the name field
  my $r = $x->search({ term => { name => 'doe' } }, 10); 

  # find top 10 documents containing 'doe' in the name,
  # AND 200 in the 'age' field
  my $r = $x->search({ term => { name => 'doe', age => '200' } }, 10); 

  # some bool query examples:
  $x->search({ 
                bool => {
                    must => [
                       { term => { "name" => 'jack' } }
                    ],
                    must_not => [
                       { term => { "name" => 'john' } }
                    ]
                }
             }, 10);

  $x->search({ 
                bool => {
                    must => [
                       { term => { "name" => 'jack' } }
                    ],
                    must_not => [
                       { term => { "name" => 'jack' } }
                    ]
                }
             }, 10);

  $x->search({ 
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
             }, 10);

  $x->search({ 
                bool => {
                    must => [
                       { 
                           bool => {  
                               must => [ { term => { name => 'jack' } } ]
                           }
                       }
                    ],
                    should => [
                       { term => { name => 'jim' } }
                    ],
                    must_not => [
                       { term => { name => 'john' } }
                    ]
                }
             }, 10);



=head1 DESCRIPTION

SHEEP is a simplistic non-flexible CLucene wrapper

It uses WhiteSpaceAnalyzer, and can work only with BooleanQueries and Term queries. It also uses boost::thread to spawn new thread for every shard when searching.

=head1 SEE ALSO

L<http://clucene.sourceforge.net/>

L<http://clucene.sourceforge.net/doc/html/namespaces.html>

L<https://github.com/jackdoe/SHEEP>

=head1 AUTHOR

Borislav Nikolov <jack@sofialondonmoskva.com>

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2013 by Borislav Nikolov

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.18.1 or,
at your option, any later version of Perl 5 you may have available.


=cut
