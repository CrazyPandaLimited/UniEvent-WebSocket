use 5.020;
use warnings;
use Test::More;
use lib 't'; use MyTest;

my $loop = UniEvent::Loop->default_loop;
my $state = 0;
my ($server, $port) = make_server();

my $cl1 = new UniEvent::Tcp();
$cl1->connect('localhost', $port);
$cl1->shutdown(sub {
	$state++;
	my $cl2 = new UniEvent::Tcp();
	$cl2->connect('localhost', $port, 1, sub {
		$state++;
		$loop->stop();
	});
});

$server->run();
$loop->run();

ok($state == 2);
done_testing();
