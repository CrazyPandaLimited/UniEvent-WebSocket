package MyTest;
use 5.012;
use warnings;
use Test::Catch;
use UniEvent::WebSocket;

XS::Loader::load();

sub make_server {
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
    my $scheme = UniEvent::WebSocket::ws_scheme();
	
	$client->connect({
        uri    => "$scheme://127.0.0.1:$port",
		ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
    });
	
	return $client;
}

1;
