use 5.020;
use warnings;
use Test::More;
use lib 't'; use MyTest;

{
    package Flogs::GetLogs::Connection;
    use parent 'UniEvent::WebSocket::ServerConnection';
}

my $loop = UE::Loop->default_loop;
my $state = 0;
my $send_cb = 0;
my ($server, $port) = make_server();

my $conns = $server->get_connections();
while (my $c = $conns->next()) {
    warn(defined($c));
}

$server->connection_event->add(sub {
    my ($serv, $conn) = @_;
    bless $conn, 'Flogs::GetLogs::Connection';
    $conn->accept_event->add(sub {
        my $conn1 = shift;
        is(ref($conn1), 'Flogs::GetLogs::Connection');
        $conn1->send(deflate => 1, payload => 'Hey!', cb => sub { $send_cb = 1;} );
    });

    warn($serv->connections_count());
    $serv->foreach_connection(sub {
        my $conn = shift;
        warn(defined($conn));
    });
    my $conns = $serv->get_connections();
    while (my $c = $conns->next()) {
        warn(defined($c));
    }
    is ($conn, $serv->get_connection($conn->id()));


});

$server->disconnection_event->add(sub {
    $state++;
    $loop->stop();
});

{
    my $client = make_client($port);
    
    $client->message_event->add(sub {
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
is $send_cb, 1;

done_testing();
