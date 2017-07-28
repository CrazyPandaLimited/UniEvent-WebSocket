use 5.020;
use Panda::Install;

write_makefile(
    NAME          => 'Panda::WebSocket::Server',
    VERSION_FROM  => 'lib/Panda/WebSocket/Server.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket/Server.pod',
    CPLUS         => 11,
    DEFINE        => $ENV{DEBUG} ? '-DDEBUG' : '',
    BIN_DEPS      => ['Panda::Event', 'Panda::WebSocket'],
    SRC           => 'src',
    INC           => '-Isrc',
    TYPEMAPS      => 'typemap',
    #BIN_SHARE => {
    #    CPLUS    => 11,
    #    INCLUDE  => {'src' => '/'},
    #    TYPEMAPS => {'typemap' => ''},
    #},
    #OPTIMIZE => '-O2 -g',
    #CC => 'clang++',
);
