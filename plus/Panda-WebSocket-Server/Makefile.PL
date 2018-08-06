use 5.020;
use Panda::Install;

my %params = (
    NAME          => 'Panda::WebSocket::Server',
    VERSION_FROM  => 'lib/Panda/WebSocket/Server.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket/Server.pod',
    CPLUS         => 11,
    DEFINE        => $ENV{DEBUG} ? '-DDEBUG' : '',
    BIN_DEPS      => ['Panda::Event', 'Panda::Lib', 'Panda::WebSocket', 'Panda::Encode::Base2N'],
    TEST_REQUIRES => {'Panda::Lib' => '1.3.7'},
    SRC           => 'src',
    INC           => '-Isrc',
    BIN_SHARE => {
        CPLUS    => 11,
        INCLUDE  => {'src' => '/'},
    },
    OPTIMIZE => '-O2',
);

if ($ENV{TEST_FULL}) {
    ($params{DEFINE}||='') .= ' -DTEST_FULL';
    $params{LIBS}[0] .= ' -lubsan';
    ($params{CCFLAGS}||='') .= ' -fsanitize=undefined';
}

write_makefile(%params);
