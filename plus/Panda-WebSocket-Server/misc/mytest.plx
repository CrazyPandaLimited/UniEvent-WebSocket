#!/usr/local/bin/perl
use 5.020;
use lib 'blib/lib', 'blib/arch', 't';
use Benchmark qw/timethis timethese/;
use Time::HiRes;
use Panda::WebSocket::Server;

say "START $$";

my $loop = new Panda::Event::Loop;

my $f = new Panda::WebSocket::Server($loop);
$f->init({
    locations => [
        {host => 'dev', port => 4680},
        {host => 'dev', port => 4681, secure => 1},
    ],
});

$f->run;

#my $t = new Panda::Event::Timer($f->loop);
#$t->timer_callback(sub {
#    $f->stop;
#    $t->stop;
#});
##$t->start(2);

say "entering loop";
$loop->run;
say "loop finished";

1;