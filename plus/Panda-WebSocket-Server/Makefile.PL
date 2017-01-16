use 5.020;
use Panda::Install;

write_makefile(
    NAME          => 'Panda::WebSocket::Server',
    VERSION_FROM  => 'lib/Panda/WebSocket/Server.pm',
    ABSTRACT_FROM => 'lib/Panda/WebSocket/Server.pod',
    PREREQ_PM     => {'Panda::Event' => 0},
    CPLUS         => 1,
    DEFINE        => $ENV{DEBUG} ? '-DDEBUG' : '',
    TEST_REQUIRES => {'Net::SSLeay' => 0},
    BIN_DEPS      => ['Panda::Event'],
    SRC           => 'src',
    INC           => '-Isrc',
    TYPEMAPS      => 'typemap',
    #BIN_SHARE => {
    #    INCLUDE  => {
    #        'src'           => '/',
    #        'libuv/include' => '/',
    #    },
    #    TYPEMAPS => {'typemap' => ''},
    #},
    OPTIMIZE => '-O2 -g',
    #CC => 'clang++',
    CCFLAGS => '-std=c++11',
);
