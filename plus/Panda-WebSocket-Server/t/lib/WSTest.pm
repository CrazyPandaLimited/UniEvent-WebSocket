use 5.020;
use warnings;

use Test::More;
use Panda::WebSocket::Server;
use Panda::WebSocket;
use Panda::Event;
use Panda::Lib qw/add_native_logger/;
use Socket;

sub make_server {
	Panda::Lib::add_native_logger(sub {
		my ($level, $msg) = @_;
		if ($level < 3) { #errorlog
			ok (0, $msg);
		}
	});

	my $loop = Panda::Event::Loop->default_loop;

	my $s = new Panda::Event::TCP();
	$s->bind('localhost',0);
	my $adr = $s->getsockname;
	my ($port, $adrrrr) = sockaddr_in ($adr);

	my $server = new Panda::WebSocket::Server();
	$server->init({
		locations => [{
			host => 'localhost',
			port => $port,
			secure => 0,
		}],
	});
	return ($server, $port);
}

sub make_client {
	my ($port) = @_;
	my $client = new Panda::WebSocket::Client();
	
	$client->connect(new Panda::WebSocket::ConnectRequest({
			uri           => "ws://localhost",
			ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
		}), 0, $port
	);
	
	return $client;
}

1;