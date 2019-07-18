package MyTest;
use 5.020;
use warnings;
use Test::Catch;
use UniEvent::WebSocket;

XS::Loader::load();

sub make_server {
	Panda::Lib::Logger::set_native_logger(sub {
		my ($level, $code, $msg) = @_;
	});

	my $loop = UniEvent::Loop->default_loop;

	my $s = new UniEvent::Tcp();
	$s->bind('127.0.0.1',0);
    my $adr = $s->sockaddr;
    my $port = $adr->port();

	my $server = new UniEvent::WebSocket::Server({
		locations => [{
			host => '127.0.0.1',
			port => $port,
		}],
	});
	return ($server, $port);
}

sub make_client {
	my ($port) = @_;
	my $client = new UniEvent::WebSocket::Client();
	
	$client->connect({
		uri    => "ws://127.0.0.1",
		ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
	}, 0, $port);
	
	return $client;
}

1;
