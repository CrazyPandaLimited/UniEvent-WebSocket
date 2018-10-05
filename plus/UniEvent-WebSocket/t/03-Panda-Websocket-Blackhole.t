use 5.020;
use warnings;
use Test::More;
use UniEvent::WebSocket::Server;
use Panda::Lib::Logger;

set_native_logger(sub {
    my ($level, $cp, $msg) = @_;
    warn("$cp $msg\n");
});

my $loop = UniEvent::Loop->default_loop;
my $stoped = 0;

{
    my $client = new UniEvent::WebSocket::Client();
	
    $client->connect_event()->add(sub {
        $stoped = 1;
        $loop->stop();
    });
    
	$client->connect({
			uri           => "ws://google.com",
			ws_key        => "dGhlIHNhbXBsZSBub25jZQ==",
		}, 0, 81
	);
    $client->close(1000);
    
    $loop->run();
    ok($stoped, 'black hole aborted');

}
done_testing();
