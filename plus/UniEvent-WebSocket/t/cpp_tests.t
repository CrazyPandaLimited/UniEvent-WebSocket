use 5.012;
use warnings;
use lib 't'; use MyTest;
use Test::Catch;
use Panda::Lib::Logger;

set_log_level(LOG_VERBOSE_DEBUG);

set_native_logger(sub {
    my ($level, $cp, $msg) = @_;
    warn($cp, $msg);
});

$SIG{PIPE} = 'IGNORE';

catch_run('[uews]');

done_testing();