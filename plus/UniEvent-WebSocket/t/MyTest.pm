use 5.020;
use warnings;
use Test::More;
use UniEvent::WebSocket;
use Socket;

plan skip_all => 'set TEST_FULL=1 to enable all tests' unless $ENV{TEST_FULL};

sub make_server {
	Panda::Lib::Logger::set_native_logger(sub {
		my ($level, $code, $msg) = @_;
	});

	my $loop = UniEvent::Loop->default_loop;

	my $s = new UniEvent::TCP();
	$s->bind('localhost',0);
    my $adr = $s->get_sockaddr;
    my $port = $adr->port();

	my $server = new UniEvent::WebSocket::Server({
		locations => [{
			host => 'localhost',
			port => $port,
		}],
	});
	return ($server, $port);
}

sub make_client {
	my ($port) = @_;
	my $client = new UniEvent::WebSocket::Client();
	
	$client->connect({
			uri           => "ws://localhost",
			ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
		}, 0, $port
	);
	
	return $client;
}

1;
