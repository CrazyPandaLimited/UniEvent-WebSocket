use 5.020;
use warnings;


use Test::More;
use Panda::WebSocket::Server;
use Panda::WebSocket;
use Panda::WebSocket::Server::Connection;
use Panda::Event;
use Panda::Lib;
use Panda::Lib::Logger;

set_native_logger(sub {
    my ($level, $cp, $msg) = @_;
    warn("$cp $msg\n");
});

my $loop = Panda::Event::Loop->default_loop;
my $stoped = 0;

{
    my $client = new Panda::WebSocket::Client();
	
    $client->connect_callback()->add(sub {
        $stoped = 1;
        $loop->stop();
    });
    
	$client->connect(new Panda::WebSocket::ConnectRequest({
			uri           => "ws://google.com",
			ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
		}), 0, 81
	);
    $client->close(1000);
    
    $loop->run();
    ok($stoped, 'black hole aborted');

}
done_testing();
