use 5.020;
use warnings;

use Test::More;
use Panda::WebSocket::Server;
use Panda::WebSocket;
use Panda::Event;
use Panda::Lib qw/add_native_logger/;
use lib 't/lib'; use WSTest;

my $loop = Panda::Event::Loop->default_loop;
my $state = 0;
my ($server, $port) = make_server();

$server->connection_callback(sub {
	my ($conn) = @_;
	$conn->accept_callback(sub {
		$conn->send_text('Hey!');
	});
});

$server->remove_connection_callback(sub {
	$state++;
	$loop->stop();
});

{
	my $client = make_client($port);

	$client->message_callback(sub {
		my ($msg) = @_;
		$state++;
		ok ($msg->payload eq 'Hey!');
		$client->close(1001);
	});

	$server->run();
	$loop->run();

}
ok($state == 2);
done_testing();