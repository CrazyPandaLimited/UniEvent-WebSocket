package MyTest;
use 5.012;
use warnings;
use Test::Catch;
use UniEvent::WebSocket;
use XLog;

XS::Loader::load();

{
    package UniEvent::WebSocket::Logger;
    use parent 'XLog::Logger';

    sub log_format {
        my ($self, $msg, $level, $module, $file, $line, $func, $formatter) = @_;
        $file = substr($file, rindex($file, '/'));
        $module = substr($module, rindex($module, '::')+2);
        my $code = "$module$file:$line";
        my $res = sprintf '%-32s %s', $code, $msg;
        say $res;
    }
}


if ($ENV{LOGGER}) {
    XLog::set_logger(UniEvent::WebSocket::Logger->new);
    XLog::set_level(XLog::WARNING);

    XLog::set_level(XLog::DEBUG, "UniEvent::WebSocket");
    XLog::set_level(XLog::INFO, "UniEvent");
}

*main::test_catch = \&test_catch;

sub test_catch {
    chdir 'clib';
    catch_run(@_);
    chdir '../';
}

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
        uri => "$scheme://127.0.0.1:$port",
        ws_key => "dGhlIHNhbXBsZSBub25jZQ==",
    });
    #$client->connect("127.0.0.1", 0, $port);
	
    return $client;
}

1;
