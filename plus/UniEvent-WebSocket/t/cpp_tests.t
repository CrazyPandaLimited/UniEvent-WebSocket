use 5.012;
use warnings;
use lib 't'; use MyTest;
use Test::Catch;
use Panda::Lib::Logger;

$SIG{PIPE} = 'IGNORE';

catch_run('[uews]');

done_testing();