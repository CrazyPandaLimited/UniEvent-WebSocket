use 5.020;
use warnings;
use Test::More;
use lib 't/lib';
use WSTest;

{
    package Flogs::GetLogs::Connection;
    use parent 'UniEvent::WebSocket::Server::Connection';
}

my $loop = UE::Loop->default_loop;
my $state = 0;
my ($server, $port) = make_server();

$server->connection_callback->add(sub {
    my ($serv, $conn) = @_;
    bless $conn, 'Flogs::GetLogs::Connection';
    $conn->accept_callback->add(sub {
        my $conn1 = shift;
        is(ref($conn1), 'Flogs::GetLogs::Connection');
        $conn1->send_text('Hey!');
    });
});

$server->disconnection_callback->add(sub {
    $state++;
    $loop->stop();
});

{
    my $client = make_client($port);
    
    $client->message_callback->add(sub {
        my ($client, $msg) = @_;
        is ref($client), "UniEvent::WebSocket::Client";
        $state++;
        ok ($msg->payload eq 'Hey!');
        $client->close(1001);
    });

    $server->run();
    $loop->run();

}
ok($state == 2);
done_testing();
